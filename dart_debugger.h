// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_DEBUGGER_H_
#define TONIC_DART_DEBUGGER_H_

#include <memory>
#include <vector>

#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "dart/runtime/include/dart_api.h"
#include "dart/runtime/include/dart_native_api.h"
#include "dart/runtime/include/dart_tools_api.h"

namespace base {
  class Lock;
}

namespace tonic {

class Monitor {
 public:
  Monitor() {
    lock_ = new base::Lock();
    condition_variable_ = new base::ConditionVariable(lock_);
  }

  ~Monitor() {
    delete condition_variable_;
    delete lock_;
  }

  void Enter() {
    lock_->Acquire();
  }

  void Exit() {
    lock_->Release();
  }

  void Notify() {
    condition_variable_->Signal();
  }

  void Wait() {
    condition_variable_->Wait();
  }

 private:
  base::Lock* lock_;
  base::ConditionVariable* condition_variable_;
  DISALLOW_COPY_AND_ASSIGN(Monitor);
};

class MonitorLocker {
 public:
  explicit MonitorLocker(Monitor* monitor) : monitor_(monitor) {
    CHECK(monitor_);
    monitor_->Enter();
  }

  virtual ~MonitorLocker();

  void Wait() {
    return monitor_->Wait();
  }

  void Notify() {
    monitor_->Notify();
  }

 private:
  Monitor* const monitor_;

  DISALLOW_COPY_AND_ASSIGN(MonitorLocker);
};

class DartDebuggerIsolate {
 public:
  DartDebuggerIsolate(Dart_IsolateId id)
      : id_(id) {
  }

  Dart_IsolateId id() const {
    return id_;
  }

  void Notify() {
    monitor_.Notify();
  }

  void MessageLoop();

 private:
  const Dart_IsolateId id_;
  Monitor monitor_;
};

class DartDebugger {
 public:
  static void InitDebugger();

 private:
  static void BptResolvedHandler(Dart_IsolateId isolate_id,
                                 intptr_t bp_id,
                                 const Dart_CodeLocation& location);

  static void PausedEventHandler(Dart_IsolateId isolate_id,
                                 intptr_t bp_id,
                                 const Dart_CodeLocation& loc);

  static void ExceptionThrownHandler(Dart_IsolateId isolate_id,
                                     Dart_Handle exception,
                                     Dart_StackTrace stack_trace);

  static void IsolateEventHandler(Dart_IsolateId isolate_id,
                                  Dart_IsolateEvent kind);

  static void NotifyIsolate(Dart_Isolate isolate);

  static intptr_t FindIsolateIndexById(Dart_IsolateId id);

  static intptr_t FindIsolateIndexByIdLocked(Dart_IsolateId id);

  static void AddIsolate(Dart_IsolateId id);

  static void RemoveIsolate(Dart_IsolateId id);

  static base::Lock* lock_;
  static std::vector<std::unique_ptr<DartDebuggerIsolate>>* isolates_;

  friend class DartDebuggerIsolate;
};

}  // namespace tonic

#endif  // TONIC_DART_DEBUGGER_H_
