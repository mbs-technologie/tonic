// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_EXCEPTION_FACTORY_H_
#define TONIC_DART_EXCEPTION_FACTORY_H_

#include <string>
#include <vector>

#include "dart/runtime/include/dart_api.h"
#include "tonic/dart_persistent_value.h"

namespace tonic {
class DartState;

class DartExceptionFactory {
 public:
  explicit DartExceptionFactory(DartState* dart_state);
  ~DartExceptionFactory();

  Dart_Handle CreateNullArgumentException(int index);
  Dart_Handle CreateException(const std::string& class_name,
                              const std::string& message);
  Dart_Handle CreateException(const std::string& class_name,
                              Dart_Handle message);

 private:
  DartState* dart_state_;
  DartPersistentValue core_library_;
};

}  // namespace tonic

#endif  // TONIC_DART_EXCEPTION_FACTORY_H_
