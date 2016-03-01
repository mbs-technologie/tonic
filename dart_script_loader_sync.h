// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_SCRIPT_LOADER_SYNC_H_
#define TONIC_DART_SCRIPT_LOADER_SYNC_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "dart/runtime/include/dart_api.h"

namespace tonic {

class DartLibraryProvider;

class DartScriptLoaderSync {
 public:
  // Blocks until |script_uri| and all of its dependencies have been loaded
  // into the current isolate.
  static void LoadScript(const std::string& script_uri,
                         DartLibraryProvider* library_provider);
};

}  // namespace tonic

#endif  // TONIC_DART_SCRIPT_LOADER_SYNC_H_
