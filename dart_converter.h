// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_CONVERTER_H_
#define TONIC_DART_CONVERTER_H_

#include <string>

#include "tonic/dart_state.h"

namespace tonic {

// DartConvert converts types back and forth from Sky to Dart. The template
// parameter |T| determines what kind of type conversion to perform.
template <typename T, typename Enable = void>
struct DartConverter {
};

// This is to work around the fact that typedefs do not create new types. If you
// have a typedef, and want it to use a different converter, specialize this
// template and override the types here.
// Ex:
//   typedef int ColorType;  // Want to use a different converter.
//   class ColorConverterType {};  // Dummy type.
//   template<> struct DartConvertType<ColorConverterType> {
//     using ConverterType = ColorConverterType;
//     using ValueType = ColorType;
//   };
template <typename T>
struct DartConverterTypes {
  using ConverterType = T;
  using ValueType = T;
};

////////////////////////////////////////////////////////////////////////////////
// Boolean

template <>
struct DartConverter<bool> {
  static Dart_Handle ToDart(bool val) { return Dart_NewBoolean(val); }

  static void SetReturnValue(Dart_NativeArguments args, bool val) {
    Dart_SetBooleanReturnValue(args, val);
  }

  static bool FromDart(Dart_Handle handle) {
    bool result = 0;
    Dart_BooleanValue(handle, &result);
    return result;
  }

  static bool FromArguments(Dart_NativeArguments args,
                            int index,
                            Dart_Handle& exception) {
    bool result = false;
    Dart_GetNativeBooleanArgument(args, index, &result);
    return result;
  }
};

////////////////////////////////////////////////////////////////////////////////
// Numbers

template <typename T>
struct DartConverterInteger {
  static Dart_Handle ToDart(T val) { return Dart_NewInteger(val); }

  static void SetReturnValue(Dart_NativeArguments args, T val) {
    Dart_SetIntegerReturnValue(args, val);
  }

  static T FromDart(Dart_Handle handle) {
    int64_t result = 0;
    Dart_IntegerToInt64(handle, &result);
    return static_cast<T>(result);
  }

  static T FromArguments(Dart_NativeArguments args,
                         int index,
                         Dart_Handle& exception) {
    int64_t result = 0;
    Dart_GetNativeIntegerArgument(args, index, &result);
    return static_cast<T>(result);
  }
};

template <>
struct DartConverter<int> : public DartConverterInteger<int> {};

template <>
struct DartConverter<unsigned> : public DartConverterInteger<unsigned> {};

template <>
struct DartConverter<long long> : public DartConverterInteger<long long> {};

template <>
struct DartConverter<unsigned long long> {
  static Dart_Handle ToDart(unsigned long long val) {
    // FIXME: WebIDL unsigned long long is guaranteed to fit into 64-bit
    // unsigned,
    // so we need a dart API for constructing an integer from uint64_t.
    DCHECK(val <= 0x7fffffffffffffffLL);
    return Dart_NewInteger(static_cast<int64_t>(val));
  }

  static void SetReturnValue(Dart_NativeArguments args,
                             unsigned long long val) {
    DCHECK(val <= 0x7fffffffffffffffLL);
    Dart_SetIntegerReturnValue(args, val);
  }

  static unsigned long long FromDart(Dart_Handle handle) {
    int64_t result = 0;
    Dart_IntegerToInt64(handle, &result);
    return result;
  }

  static unsigned long long FromArguments(Dart_NativeArguments args,
                                          int index,
                                          Dart_Handle& exception) {
    int64_t result = 0;
    Dart_GetNativeIntegerArgument(args, index, &result);
    return result;
  }
};

template <typename T>
struct DartConverterFloatingPoint {
  static Dart_Handle ToDart(T val) { return Dart_NewDouble(val); }

  static void SetReturnValue(Dart_NativeArguments args, T val) {
    Dart_SetDoubleReturnValue(args, val);
  }

  static T FromDart(Dart_Handle handle) {
    double result = 0;
    Dart_DoubleValue(handle, &result);
    return result;
  }

  static T FromArguments(Dart_NativeArguments args,
                         int index,
                         Dart_Handle& exception) {
    double result = 0;
    Dart_GetNativeDoubleArgument(args, index, &result);
    return result;
  }
};

template <>
struct DartConverter<float> : public DartConverterFloatingPoint<float> {};

template <>
struct DartConverter<double> : public DartConverterFloatingPoint<double> {};

////////////////////////////////////////////////////////////////////////////////
// std::string support (slower, but more convienent for some clients)

inline Dart_Handle StdStringToDart(const std::string& val) {
  return Dart_NewStringFromUTF8(reinterpret_cast<const uint8_t*>(val.data()),
                                val.length());
}

inline std::string StdStringFromDart(Dart_Handle handle) {
  uint8_t* data = nullptr;
  intptr_t length = 0;
  Dart_Handle r = Dart_StringToUTF8(handle, &data, &length);
  if (Dart_IsError(r)) {
    return std::string();
  }
  return std::string(reinterpret_cast<const char*>(data),
                     static_cast<size_t>(length));
}


// Alias Dart_NewStringFromCString for less typing.
inline Dart_Handle ToDart(const char* val) {
  return Dart_NewStringFromCString(val);
}

}  // namespace tonic

#endif  // TONIC_DART_CONVERTER_H_
