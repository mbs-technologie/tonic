// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tonic/dart_library_provider_files.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "tonic/dart_converter.h"

namespace tonic {
namespace {

void CopyComplete(base::FilePath file, bool success) {
  if (!success)
    LOG(ERROR) << "Failed to load " << file.AsUTF8Unsafe();
}

base::FilePath SimplifyPath(const base::FilePath& path) {
  std::vector<base::FilePath::StringType> components;
  path.GetComponents(&components);
  auto it = components.begin();
  base::FilePath result(*it++);
  for (; it != components.end(); it++) {
    auto& component = *it;
    if (component == base::FilePath::kCurrentDirectory)
      continue;
    if (component == base::FilePath::kParentDirectory)
      result = result.DirName();
    else
      result = result.Append(component);
  }
  return result;
}

}  // namespace

DartLibraryProviderFiles::DartLibraryProviderFiles(
    const base::FilePath& package_root)
    : package_root_(package_root) {
    CHECK(base::DirectoryExists(package_root_)) << "Invalid --package-root "
      << "\"" << package_root_.LossyDisplayName() << "\"";
}

DartLibraryProviderFiles::~DartLibraryProviderFiles() {
}

void DartLibraryProviderFiles::GetLibraryAsStream(
    const std::string& name,
    DataPipeConsumerCallback callback) {
  mojo::DataPipe pipe;
  callback.Run(pipe.consumer_handle.Pass());

  base::FilePath source(name);
  scoped_refptr<base::SingleThreadTaskRunner> runner =
      base::MessageLoop::current()->task_runner();
  mojo::common::CopyFromFile(source, pipe.producer_handle.Pass(), 0,
                             runner.get(), base::Bind(&CopyComplete, source));
}

std::string DartLibraryProviderFiles::CanonicalizePackageURL(std::string url) {
  DCHECK(base::StartsWith(url, "package:", base::CompareCase::SENSITIVE));
  base::ReplaceFirstSubstringAfterOffset(&url, 0, "package:", "");
  return package_root_.Append(url).AsUTF8Unsafe();
}

Dart_Handle DartLibraryProviderFiles::CanonicalizeURL(Dart_Handle library,
                                                      Dart_Handle url) {
  std::string string = StdStringFromDart(url);
  if (base::StartsWith(string, "dart:", base::CompareCase::SENSITIVE))
    return url;
  if (base::StartsWith(string, "package:", base::CompareCase::SENSITIVE))
    return StdStringToDart(CanonicalizePackageURL(string));
  base::FilePath base_path(StdStringFromDart(Dart_LibraryUrl(library)));
  base::FilePath resolved_path = base_path.DirName().Append(string);
  base::FilePath normalized_path = SimplifyPath(resolved_path);
  return StdStringToDart(normalized_path.AsUTF8Unsafe());
}

}  // namespace tonic
