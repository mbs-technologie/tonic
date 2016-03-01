// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tonic/dart_script_loader_sync.h"

#include "base/callback.h"
#include "base/run_loop.h"
#include "base/trace_event/trace_event.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "tonic/dart_api_scope.h"
#include "tonic/dart_converter.h"
#include "tonic/dart_dependency_catcher.h"
#include "tonic/dart_error.h"
#include "tonic/dart_isolate_scope.h"
#include "tonic/dart_library_loader.h"
#include "tonic/dart_library_provider.h"
#include "tonic/dart_state.h"

namespace tonic {

static void BlockWaitingForDependencies(
    tonic::DartLibraryLoader* loader,
    const std::unordered_set<tonic::DartDependency*>& dependencies) {
  {
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::MessageLoop::current()->task_runner();
    base::RunLoop run_loop;
    task_runner->PostTask(
        FROM_HERE,
        base::Bind(
            &tonic::DartLibraryLoader::WaitForDependencies,
            base::Unretained(loader),
            dependencies,
            base::Bind(
               base::IgnoreResult(&base::SingleThreadTaskRunner::PostTask),
               task_runner.get(), FROM_HERE,
               run_loop.QuitClosure())));
    run_loop.Run();
  }
}


static void InnerLoadScript(
    const std::string& script_uri,
    tonic::DartLibraryProvider* library_provider) {
  // When spawning isolates, Dart expects the script loading to be completed
  // before returning from the isolate creation callback. The mojo dart
  // controller also expects the isolate to be finished loading a script
  // before the isolate creation callback returns.

  // We block here by creating a nested message pump and waiting for the load
  // to complete.

  DCHECK(base::MessageLoop::current() != nullptr);
  base::MessageLoop::ScopedNestableTaskAllower allow(
      base::MessageLoop::current());

  // Initiate the load.
  auto dart_state = tonic::DartState::Current();
  DCHECK(library_provider != nullptr);
  tonic::DartLibraryLoader& loader = dart_state->library_loader();
  loader.set_library_provider(library_provider);
  std::unordered_set<tonic::DartDependency*> dependencies;
  {
    tonic::DartDependencyCatcher dependency_catcher(loader);
    loader.LoadScript(script_uri);
    // Copy dependencies before dependency_catcher goes out of scope.
    dependencies = std::unordered_set<tonic::DartDependency*>(
        dependency_catcher.dependencies());
  }

  // Run inner message loop.
  BlockWaitingForDependencies(&loader, dependencies);

  // Finalize loading.
  tonic::LogIfError(Dart_FinalizeLoading(true));
}

void DartScriptLoaderSync::LoadScript(
    const std::string& script_uri,
    DartLibraryProvider* library_provider) {
  if (base::MessageLoop::current() == nullptr) {
    // Threads running on the Dart thread pool may not have a message loop,
    // we rely on a message loop during loading. Create a temporary one
    // here.
    base::MessageLoop message_loop(mojo::common::MessagePumpMojo::Create());
    InnerLoadScript(script_uri, library_provider);
  } else {
    // Thread has a message loop, use it.
    InnerLoadScript(script_uri, library_provider);
  }
}

}  // namespace tonic
