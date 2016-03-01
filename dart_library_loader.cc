// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tonic/dart_library_loader.h"

#include "base/callback.h"
#include "base/trace_event/trace_event.h"
#include "mojo/data_pipe_utils/data_pipe_drainer.h"
#include "tonic/dart_api_scope.h"
#include "tonic/dart_converter.h"
#include "tonic/dart_dependency_catcher.h"
#include "tonic/dart_error.h"
#include "tonic/dart_isolate_scope.h"
#include "tonic/dart_library_provider.h"
#include "tonic/dart_state.h"

using mojo::common::DataPipeDrainer;

namespace tonic {

namespace {

// Helper to erase a T* from a container of std::unique_ptr<T>s.
template<typename T, typename C>
void EraseUniquePtr(C& container, T* item) {
  std::unique_ptr<T> key = std::unique_ptr<T>(item);
  container.erase(key);
  key.release();
}

}

// A DartLibraryLoader::Job represents a network load. It fetches data from the
// network and buffers the data in std::vector. To cancel the job, delete this
// object.
class DartLibraryLoader::Job : public DartDependency,
                               public DataPipeDrainer::Client {
 public:
  Job(DartLibraryLoader* loader, const std::string& name)
      : loader_(loader), name_(name), weak_factory_(this) {
    loader->library_provider()->GetLibraryAsStream(
        name, base::Bind(&Job::OnStreamAvailable, weak_factory_.GetWeakPtr()));
  }

  const std::string& name() const { return name_; }

 protected:
  DartLibraryLoader* loader_;
  // TODO(abarth): Should we be using SharedBuffer to buffer the data?
  std::vector<uint8_t> buffer_;

 private:
  void OnStreamAvailable(mojo::ScopedDataPipeConsumerHandle pipe) {
    if (!pipe.is_valid()) {
      loader_->DidFailJob(this);
      return;
    }
    drainer_ = std::unique_ptr<DataPipeDrainer>(
        new DataPipeDrainer(this, pipe.Pass()));
  }

  // DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    buffer_.insert(buffer_.end(), bytes, bytes + num_bytes);
  }
  // Subclasses must implement OnDataComplete.

  std::string name_;
  std::unique_ptr<DataPipeDrainer> drainer_;

  base::WeakPtrFactory<Job> weak_factory_;
};

class DartLibraryLoader::ImportJob : public Job {
 public:
  ImportJob(DartLibraryLoader* loader,
            const std::string& name,
            bool load_script) : Job(loader, name), load_script_(load_script) {
    TRACE_EVENT_ASYNC_BEGIN1("sky", "DartLibraryLoader::ImportJob", this, "url",
                             name);
  }

  bool load_script() const { return load_script_; }

 private:
  // DataPipeDrainer::Client
  void OnDataComplete() override {
    TRACE_EVENT_ASYNC_END0("sky", "DartLibraryLoader::ImportJob", this);
    loader_->DidCompleteImportJob(this, buffer_);
  }

  bool load_script_;
};

class DartLibraryLoader::SourceJob : public Job {
 public:
  SourceJob(DartLibraryLoader* loader, const std::string& name, Dart_Handle library)
      : Job(loader, name), library_(loader->dart_state(), library) {
    TRACE_EVENT_ASYNC_BEGIN1("sky", "DartLibraryLoader::SourceJob", this, "url",
                             name);
  }

  Dart_PersistentHandle library() const { return library_.value(); }

 private:
  // DataPipeDrainer::Client
  void OnDataComplete() override {
    TRACE_EVENT_ASYNC_END0("sky", "DartLibraryLoader::SourceJob", this);
    loader_->DidCompleteSourceJob(this, buffer_);
  }

  DartPersistentValue library_;
};

// A DependencyWatcher represents a request to watch for when a given set of
// dependencies (either libraries or parts of libraries) have finished loading.
// When the dependencies are satisfied (including transitive dependencies), then
// the |callback| will be invoked.
class DartLibraryLoader::DependencyWatcher {
 public:
  DependencyWatcher(const std::unordered_set<DartDependency*>& dependencies,
                    const base::Closure& callback)
      : dependencies_(dependencies), callback_(callback) {
    DCHECK(!dependencies_.empty());
  }

  bool DidResolveDependency(
      DartDependency* resolved_dependency,
      const std::unordered_set<DartDependency*>& new_dependencies) {
    const auto& it = dependencies_.find(resolved_dependency);
    if (it == dependencies_.end())
      return false;
    dependencies_.erase(it);
    for (const auto& dependency : new_dependencies)
      dependencies_.insert(dependency);
    return dependencies_.empty();
  }

  const base::Closure& callback() const { return callback_; }

 private:
  std::unordered_set<DartDependency*> dependencies_;
  base::Closure callback_;
};

// A WatcherSignaler is responsible for signaling DependencyWatchers when their
// dependencies resolve and for calling the DependencyWatcher's callback. We use
// a separate object of this task because we want to carefully manage when we
// call the callbacks, which can call into us again reentrantly.
//
// WatcherSignaler is designed to be placed on the stack as a RAII. After its
// destructor runs, we might have executed aribitrary script.
class DartLibraryLoader::WatcherSignaler {
 public:
  WatcherSignaler(DartLibraryLoader& loader,
                  DartDependency* resolved_dependency)
      : loader_(loader),
        catcher_(std::unique_ptr<DartDependencyCatcher>(
                    new DartDependencyCatcher(loader))),
        resolved_dependency_(resolved_dependency) {}

  ~WatcherSignaler() {
    std::vector<DependencyWatcher*> completed_watchers;
    for (const auto& watcher : loader_.dependency_watchers_) {
      if (watcher->DidResolveDependency(resolved_dependency_,
                                        catcher_->dependencies()))
        completed_watchers.push_back(watcher.get());
    }

    // Notice that we remove the dependency catcher and extract all the
    // callbacks before running any of them. We don't want to be re-entered
    // below the callbacks and end up in an inconsistent state.
    catcher_.reset();
    std::vector<base::Closure> callbacks;
    for (const auto& watcher : completed_watchers) {
      callbacks.push_back(watcher->callback());
      EraseUniquePtr(loader_.dependency_watchers_, watcher);
    }

    // Finally, run all the callbacks while touching only data on the stack.
    for (const auto& callback : callbacks)
      callback.Run();
  }

 private:
  DartLibraryLoader& loader_;
  std::unique_ptr<DartDependencyCatcher> catcher_;
  DartDependency* resolved_dependency_;
};

DartLibraryLoader::DartLibraryLoader(DartState* dart_state)
    : dart_state_(dart_state),
      library_provider_(nullptr),
      dependency_catcher_(nullptr),
      magic_number_(nullptr),
      magic_number_len_(0),
      error_during_loading_(false) {
}

DartLibraryLoader::~DartLibraryLoader() {
}

Dart_Handle DartLibraryLoader::HandleLibraryTag(Dart_LibraryTag tag,
                                                Dart_Handle library,
                                                Dart_Handle url) {
  DCHECK(Dart_IsLibrary(library));
  DCHECK(Dart_IsString(url));
  if (tag == Dart_kCanonicalizeUrl)
    return DartState::Current()->library_loader().CanonicalizeURL(library, url);
  if (tag == Dart_kImportTag) {
    return DartState::Current()->library_loader().Import(library, url);
  }
  if (tag == Dart_kSourceTag) {
    return DartState::Current()->library_loader().Source(library, url);
  }
  DCHECK(false);
  return Dart_NewApiError("Unknown library tag.");
}

void DartLibraryLoader::WaitForDependencies(
    const std::unordered_set<DartDependency*>& dependencies,
    const base::Closure& callback) {
  if (dependencies.empty())
    return callback.Run();
  dependency_watchers_.insert(
      std::unique_ptr<DependencyWatcher>(
          new DependencyWatcher(dependencies, callback)));
}

void DartLibraryLoader::LoadLibrary(const std::string& name) {
  const auto& result = pending_libraries_.insert(std::make_pair(name, nullptr));
  if (result.second) {
    // New entry.
    std::unique_ptr<Job> job = std::unique_ptr<Job>(
        new ImportJob(this, name, false));
    result.first->second = job.get();
    jobs_.insert(std::move(job));
  }
  if (dependency_catcher_)
    dependency_catcher_->AddDependency(result.first->second);
}

void DartLibraryLoader::LoadScript(const std::string& name) {
  const auto& result = pending_libraries_.insert(std::make_pair(name, nullptr));
  if (result.second) {
    // New entry.
    std::unique_ptr<Job> job = std::unique_ptr<Job>(
        new ImportJob(this, name, true));
    result.first->second = job.get();
    jobs_.insert(std::move(job));
  }
  if (dependency_catcher_)
    dependency_catcher_->AddDependency(result.first->second);
}

Dart_Handle DartLibraryLoader::Import(Dart_Handle library, Dart_Handle url) {
  LoadLibrary(StdStringFromDart(url));
  return Dart_True();
}

Dart_Handle DartLibraryLoader::Source(Dart_Handle library, Dart_Handle url) {
  std::unique_ptr<Job> job = std::unique_ptr<Job>(
      new SourceJob(this, StdStringFromDart(url), library));
  if (dependency_catcher_)
    dependency_catcher_->AddDependency(job.get());
  jobs_.insert(std::move(job));
  return Dart_True();
}

Dart_Handle DartLibraryLoader::CanonicalizeURL(Dart_Handle library,
                                               Dart_Handle url) {
  DCHECK(library_provider_ != nullptr);
  return library_provider_->CanonicalizeURL(library, url);
}

const uint8_t* DartLibraryLoader::SniffForMagicNumber(
    const uint8_t* text_buffer, intptr_t* buffer_len, bool* is_snapshot) {
  if (magic_number_ == nullptr) {
    *is_snapshot = false;
    return text_buffer;
  }
  for (intptr_t i = 0; i < magic_number_len_; i++) {
    if (text_buffer[i] != magic_number_[i]) {
      *is_snapshot = false;
      return text_buffer;
    }
  }
  *is_snapshot = true;
  DCHECK_GT(*buffer_len, magic_number_len_);
  *buffer_len -= magic_number_len_;
  return text_buffer + magic_number_len_;
}

void DartLibraryLoader::DidCompleteImportJob(
    ImportJob* job,
    const std::vector<uint8_t>& buffer) {
  TRACE_EVENT1("sky", "DartLibraryLoader::DidCompleteImportJob", "url",
               job->name());
  DartIsolateScope scope(dart_state_->isolate());
  DartApiScope api_scope;

  WatcherSignaler watcher_signaler(*this, job);

  Dart_Handle result;

  if (job->load_script()) {
    // Sniff for magic number. Load script from snapshot if found.
    const uint8_t* buf = buffer.data();
    intptr_t len = buffer.size();
    bool is_snapshot = false;
    buf = SniffForMagicNumber(buf, &len, &is_snapshot);
    if (is_snapshot) {
      result = Dart_LoadScriptFromSnapshot(buf, len);
    } else {
      result = Dart_LoadScript(
          StdStringToDart(job->name()), Dart_NewStringFromUTF8(buf, len), 0, 0);
    }
  } else {
    result = Dart_LoadLibrary(
        StdStringToDart(job->name()),
        Dart_NewStringFromUTF8(buffer.data(), buffer.size()), 0, 0);
  }

  if (Dart_IsError(result)) {
    LOG(ERROR) << "Error Loading " << job->name() << " "
        << Dart_GetError(result);
    error_during_loading_ = true;
  }

  pending_libraries_.erase(job->name());
  EraseUniquePtr<Job>(jobs_, job);
}

void DartLibraryLoader::DidCompleteSourceJob(
    SourceJob* job,
    const std::vector<uint8_t>& buffer) {
  TRACE_EVENT1("sky", "DartLibraryLoader::DidCompleteSourceJob", "url",
               job->name());
  DartIsolateScope scope(dart_state_->isolate());
  DartApiScope api_scope;

  WatcherSignaler watcher_signaler(*this, job);

  Dart_Handle result = Dart_LoadSource(
      Dart_HandleFromPersistent(job->library()),
      StdStringToDart(job->name()),
      Dart_NewStringFromUTF8(buffer.data(), buffer.size()), 0, 0);

  if (Dart_IsError(result)) {
    LOG(ERROR) << "Error Loading " << job->name() << " "
        << Dart_GetError(result);
    error_during_loading_ = true;
  }

  EraseUniquePtr<Job>(jobs_, job);
}

void DartLibraryLoader::DidFailJob(Job* job) {
  DartIsolateScope scope(dart_state_->isolate());
  DartApiScope api_scope;

  WatcherSignaler watcher_signaler(*this, job);

  LOG(ERROR) << "Library Load failed: " << job->name();
  // TODO(eseidel): Call Dart_LibraryHandleError in the SourceJob case?
  error_during_loading_ = true;

  EraseUniquePtr<Job>(jobs_, job);
}

}  // namespace tonic
