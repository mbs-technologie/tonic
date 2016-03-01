// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tonic/dart_state.h"

#include "tonic/dart_class_library.h"
#include "tonic/dart_converter.h"
#include "tonic/dart_exception_factory.h"
#include "tonic/dart_library_loader.h"
#include "tonic/dart_message_handler.h"
#include "tonic/dart_timer_heap.h"

namespace tonic {

DartState::Scope::Scope(DartState* dart_state) : scope_(dart_state->isolate()) {
}

DartState::Scope::~Scope() {
}

DartState::DartState()
    : isolate_(nullptr),
      class_library_(std::unique_ptr<DartClassLibrary>(new DartClassLibrary)),
      exception_factory_(std::unique_ptr<DartExceptionFactory>(
          new DartExceptionFactory(this))),
      library_loader_(std::unique_ptr<DartLibraryLoader>(
          new DartLibraryLoader(this))),
      timer_heap_(std::unique_ptr<DartTimerHeap>(
          new DartTimerHeap())),
      message_handler_(std::unique_ptr<DartMessageHandler>(
          new DartMessageHandler())),
      weak_factory_(this) {
}

DartState::~DartState() {
}

void DartState::SetIsolate(Dart_Isolate isolate) {
  isolate_ = isolate;
  if (!isolate_)
    return;

  DidSetIsolate();
}

DartState* DartState::From(Dart_Isolate isolate) {
  return static_cast<DartState*>(Dart_IsolateData(isolate));
}

DartState* DartState::Current() {
  return static_cast<DartState*>(Dart_CurrentIsolateData());
}

base::WeakPtr<DartState> DartState::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace tonic
