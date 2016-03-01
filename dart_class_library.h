// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_CLASS_LIBRARY_H_
#define TONIC_DART_CLASS_LIBRARY_H_

#include <unordered_map>
#include "base/macros.h"
#include "dart/runtime/include/dart_api.h"
#include "tonic/dart_class_provider.h"

namespace tonic {
struct DartWrapperInfo;

class DartClassLibrary {
 public:
  explicit DartClassLibrary();
  ~DartClassLibrary();

  void set_provider(DartClassProvider* provider) { provider_ = provider; }
  Dart_PersistentHandle GetClass(const DartWrapperInfo& info);

 private:
  DartClassProvider* provider_;
  std::unordered_map<const DartWrapperInfo*, Dart_PersistentHandle> cache_;

  DISALLOW_COPY_AND_ASSIGN(DartClassLibrary);
};

}  // namespace tonic

#endif  // TONIC_DART_CLASS_LIBRARY_H_
