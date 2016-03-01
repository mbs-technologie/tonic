// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_CLASS_PROVIDER_H_
#define TONIC_DART_CLASS_PROVIDER_H_

#include "dart/runtime/include/dart_api.h"

namespace tonic {

class DartClassProvider {
 public:
  virtual Dart_Handle GetClassByName(const char* class_name) = 0;

 protected:
  virtual ~DartClassProvider();
};

}  // namespace tonic

#endif  // TONIC_DART_CLASS_PROVIDER_H_
