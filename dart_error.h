// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_ERROR_H_
#define TONIC_DART_ERROR_H_

#include "dart/runtime/include/dart_api.h"

namespace tonic {

namespace DartError {
extern const char kInvalidArgument[];
}  // namespace DartError

bool LogIfError(Dart_Handle handle);

}  // namespace tonic

#endif  // TONIC_DART_ERROR_H_

