// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_VM_H_
#define TONIC_DART_VM_H_

#include <vector>

#include "dart/runtime/include/dart_api.h"

namespace tonic {

// Initialize and Cleanup the Dart VM.
class DartVM {
 public:
  struct Config {
    Config();

    const uint8_t* vm_isolate_snapshot;
    const uint8_t* instructions_snapshot;
    const uint8_t* data_snapshot;
    Dart_IsolateCreateCallback create;
    Dart_IsolateInterruptCallback interrupt;
    Dart_IsolateUnhandledExceptionCallback unhandled_exception;
    Dart_IsolateShutdownCallback shutdown;
    Dart_FileOpenCallback file_open;
    Dart_FileReadCallback file_read;
    Dart_FileWriteCallback file_write;
    Dart_FileCloseCallback file_close;
    Dart_EntropySource entropy_source;
    Dart_GetVMServiceAssetsArchive get_service_assets;
  };

  // Initialize the Dart VM using |config| and |flags|. Returns |false| on
  // error and then the error message can be retrieved by calling |error|.
  static bool Initialize(const Config& config,
                         const std::vector<const char*>& flags);

  // Cleanup the Dart VM. Returns |false| on error and then the error message
  // can be retrieved by calling |error|.
  static bool Cleanup();

  // Returns the error string after a failing call to |Cleanup| or |Initialize|.
  static const char* error();
};

}  // namespace tonic

#endif  // TONIC_DART_VM_H_
