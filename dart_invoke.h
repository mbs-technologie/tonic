// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_INVOKE_H_
#define TONIC_DART_INVOKE_H_

#include "dart/runtime/include/dart_api.h"

namespace tonic {

bool DartInvokeAppField(Dart_Handle target, Dart_Handle name,
                        int number_of_arguments,
                        Dart_Handle* arguments);

bool DartInvokeAppClosure(Dart_Handle closure,
                          int number_of_arguments,
                          Dart_Handle* arguments);

}  // namespace tonic

#endif  // TONIC_DART_INVOKE_H_

