// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tonic/dart_vm.h"

#include "base/logging.h"
#include "tonic/dart_debugger.h"

namespace tonic {

static char* dart_vm_error_ = NULL;

DartVM::Config::Config() {
  vm_isolate_snapshot = nullptr;
  instructions_snapshot = nullptr;
  data_snapshot = nullptr;
  create = nullptr;
  interrupt = nullptr;
  unhandled_exception = nullptr;
  shutdown = nullptr;
  file_open = nullptr;
  file_read = nullptr;
  file_write = nullptr;
  file_close = nullptr;
  entropy_source = nullptr;
  get_service_assets = nullptr;
}

bool DartVM::Initialize(const Config& config,
                        const std::vector<const char*>& flags) {
  CHECK(dart_vm_error_ == NULL);

  // Set the flags before calling Dart_Initialize.
  if (!Dart_SetVMFlags(flags.size(), const_cast<const char**>(flags.data()))) {
    dart_vm_error_ = strdup("Error in Dart_SetVMFlags.");
    LOG(ERROR) << "Dart_SetVMFlags: " << dart_vm_error_;
    return false;
  }

  // Initialize debugger before calling Dart_Initialize.
  DartDebugger::InitDebugger();

  dart_vm_error_ = Dart_Initialize(config.vm_isolate_snapshot,
                                   config.instructions_snapshot,
                                   config.data_snapshot,
                                   config.create,
                                   config.interrupt,
                                   config.unhandled_exception,
                                   config.shutdown,
                                   config.file_open,
                                   config.file_read,
                                   config.file_write,
                                   config.file_close,
                                   config.entropy_source,
                                   config.get_service_assets);
  if (dart_vm_error_ != NULL) {
    LOG(ERROR) << "Dart_Initialize: " << dart_vm_error_;
  }
  return dart_vm_error_ == NULL;
}

bool DartVM::Cleanup() {
  CHECK(dart_vm_error_ == NULL);
  dart_vm_error_ = Dart_Cleanup();
  if (dart_vm_error_ != NULL) {
    LOG(ERROR) << "Dart_Cleanup: " << dart_vm_error_;
  }
  return dart_vm_error_ == NULL;
}

const char* DartVM::error() {
  CHECK(dart_vm_error_ != NULL);
  return dart_vm_error_;
}

}  // namespace tonic
