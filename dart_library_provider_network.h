// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_LIBRARY_PROVIDER_NETWORK_H_
#define TONIC_DART_LIBRARY_PROVIDER_NETWORK_H_

#include <unordered_set>

#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "tonic/dart_library_provider.h"

namespace tonic {

class DartLibraryProviderNetwork : public DartLibraryProvider {
 public:
  explicit DartLibraryProviderNetwork(mojo::NetworkService* network_service);
  ~DartLibraryProviderNetwork() override;

  mojo::NetworkService* network_service() const { return network_service_; }

 protected:
  // |DartLibraryProvider| implementation:
  void GetLibraryAsStream(const std::string& name,
                          DataPipeConsumerCallback callback) override;
  Dart_Handle CanonicalizeURL(Dart_Handle library, Dart_Handle url) override;

 private:
  class Job;

  mojo::NetworkService* network_service_;
  std::unordered_set<std::unique_ptr<Job>> jobs_;

  DISALLOW_COPY_AND_ASSIGN(DartLibraryProviderNetwork);
};

}  // namespace tonic

#endif  // TONIC_DART_LIBRARY_PROVIDER_NETWORK_H_
