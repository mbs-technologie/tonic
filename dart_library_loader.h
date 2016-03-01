// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_LIBRARY_LOADER_H_
#define TONIC_DART_LIBRARY_LOADER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "dart/runtime/include/dart_api.h"

namespace tonic {
class DartDependency;
class DartDependencyCatcher;
class DartLibraryProvider;
class DartState;

// TODO(abarth): This class seems more complicated than it needs to be. Is
// there some way of simplifying this system? For example, we have a bunch
// of inner classes that could potentially be factored out in some other way.
class DartLibraryLoader {
 public:
  explicit DartLibraryLoader(DartState* dart_state);
  ~DartLibraryLoader();

  // TODO(dart): This can be called both on the main thread from application
  // solates or from the handle watcher isolate thread.
  static Dart_Handle HandleLibraryTag(Dart_LibraryTag tag,
                                      Dart_Handle library,
                                      Dart_Handle url);

  void LoadLibrary(const std::string& name);
  void LoadScript(const std::string& name);

  void WaitForDependencies(
      const std::unordered_set<DartDependency*>& dependencies,
      const base::Closure& callback);

  void set_dependency_catcher(DartDependencyCatcher* dependency_catcher) {
    DCHECK(!dependency_catcher_ || !dependency_catcher);
    dependency_catcher_ = dependency_catcher;
  }

  DartState* dart_state() const { return dart_state_; }

  DartLibraryProvider* library_provider() const { return library_provider_; }

  // The |DartLibraryProvider| must outlive the |DartLibraryLoader|.
  void set_library_provider(DartLibraryProvider* library_provider) {
    library_provider_ = library_provider;
  }

  bool error_during_loading() const {
    return error_during_loading_;
  }

  // If one is needed by the embedder, this sets the magic number used to
  // distinguish snapshots from scripts. If no magic number is set, the
  // DartLibraryLoader always assumes that the targets of LoadLibrary and
  // LoadScript are Dart source, and not snapshots.
  void set_magic_number(const uint8_t* magic_number, intptr_t len) {
    magic_number_ = magic_number;
    magic_number_len_ = len;
  }

 private:
  class Job;
  class ImportJob;
  class SourceJob;
  class DependencyWatcher;
  class WatcherSignaler;

  Dart_Handle Import(Dart_Handle library, Dart_Handle url);
  Dart_Handle Source(Dart_Handle library, Dart_Handle url);
  Dart_Handle CanonicalizeURL(Dart_Handle library, Dart_Handle url);
  void DidCompleteImportJob(ImportJob* job, const std::vector<uint8_t>& buffer);
  void DidCompleteSourceJob(SourceJob* job, const std::vector<uint8_t>& buffer);
  void DidFailJob(Job* job);

  const uint8_t* SniffForMagicNumber(
    const uint8_t* text_buffer, intptr_t* buffer_len, bool* is_snapshot);

  DartState* dart_state_;
  DartLibraryProvider* library_provider_;
  std::unordered_map<std::string, Job*> pending_libraries_;
  std::unordered_set<std::unique_ptr<Job>> jobs_;
  std::unordered_set<std::unique_ptr<DependencyWatcher>> dependency_watchers_;
  DartDependencyCatcher* dependency_catcher_;
  const uint8_t* magic_number_;
  intptr_t magic_number_len_;
  bool error_during_loading_;

  DISALLOW_COPY_AND_ASSIGN(DartLibraryLoader);
};

}  // namespace tonic

#endif  // TONIC_DART_LIBRARY_LOADER_H_
