// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_CONVERTER_WTF_H_
#define TONIC_DART_CONVERTER_WTF_H_

#include <string>
#include "sky/engine/wtf/text/StringUTF8Adaptor.h"
#include "sky/engine/wtf/text/WTFString.h"
#include "tonic/dart_converter.h"
#include "tonic/dart_string.h"
#include "tonic/dart_string_cache.h"
#include "tonic/dart_value.h"

namespace tonic {

////////////////////////////////////////////////////////////////////////////////
// Strings

template <>
struct DartConverter<String> {
  static Dart_Handle ToDart(DartState* state, const String& val) {
    if (val.isEmpty())
      return Dart_EmptyString();
    return Dart_HandleFromWeakPersistent(state->string_cache().Get(val.impl()));
  }

  static void SetReturnValue(Dart_NativeArguments args,
                             const String& val,
                             bool auto_scope = true) {
    // TODO(abarth): What should we do with auto_scope?
    if (val.isEmpty()) {
      Dart_SetReturnValue(args, Dart_EmptyString());
      return;
    }
    DartState* state = DartState::Current();
    Dart_SetWeakHandleReturnValue(args, state->string_cache().Get(val.impl()));
  }

  static void SetReturnValueWithNullCheck(Dart_NativeArguments args,
                                          const String& val,
                                          bool auto_scope = true) {
    if (val.isNull())
      Dart_SetReturnValue(args, Dart_Null());
    else
      SetReturnValue(args, val, auto_scope);
  }

  static String FromDart(Dart_Handle handle) {
    intptr_t char_size = 0;
    intptr_t length = 0;
    void* peer = nullptr;
    Dart_Handle result =
        Dart_StringGetProperties(handle, &char_size, &length, &peer);
    if (peer)
      return String(static_cast<StringImpl*>(peer));
    if (Dart_IsError(result))
      return String();
    return ExternalizeDartString(handle);
  }

  static String FromArguments(Dart_NativeArguments args,
                              int index,
                              Dart_Handle& exception,
                              bool auto_scope = true) {
    // TODO(abarth): What should we do with auto_scope?
    void* peer = nullptr;
    Dart_Handle handle = Dart_GetNativeStringArgument(args, index, &peer);
    if (peer)
      return reinterpret_cast<StringImpl*>(peer);
    if (Dart_IsError(handle))
      return String();
    return ExternalizeDartString(handle);
  }

  static String FromArgumentsWithNullCheck(Dart_NativeArguments args,
                                           int index,
                                           Dart_Handle& exception,
                                           bool auto_scope = true) {
    // TODO(abarth): What should we do with auto_scope?
    void* peer = nullptr;
    Dart_Handle handle = Dart_GetNativeStringArgument(args, index, &peer);
    if (peer)
      return reinterpret_cast<StringImpl*>(peer);
    if (Dart_IsError(handle) || Dart_IsNull(handle))
      return String();
    return ExternalizeDartString(handle);
  }
};

template <>
struct DartConverter<AtomicString> {
  static Dart_Handle ToDart(DartState* state, const AtomicString& val) {
    return DartConverter<String>::ToDart(state, val.string());
  }
};

////////////////////////////////////////////////////////////////////////////////
// Collections

template <typename T>
struct DartConverter<Vector<T>> {
  using ValueType = typename DartConverterTypes<T>::ValueType;
  using ConverterType = typename DartConverterTypes<T>::ConverterType;

  static Dart_Handle ToDart(const Vector<ValueType>& val) {
    Dart_Handle list = Dart_NewList(val.size());
    if (Dart_IsError(list))
      return list;
    for (size_t i = 0; i < val.size(); i++) {
      Dart_Handle result =
          Dart_ListSetAt(list, i,
                         DartConverter<ConverterType>::ToDart(val[i]));
      if (Dart_IsError(result))
        return result;
    }
    return list;
  }

  static Vector<ValueType> FromDart(Dart_Handle handle) {
    Vector<ValueType> result;
    if (!Dart_IsList(handle))
      return result;
    intptr_t length = 0;
    Dart_ListLength(handle, &length);
    result.reserveCapacity(length);
    for (intptr_t i = 0; i < length; ++i) {
      Dart_Handle item = Dart_ListGetAt(handle, i);
      DCHECK(!Dart_IsError(item));
      DCHECK(item);
      result.append(DartConverter<ConverterType>::FromDart(item));
    }
    return result;
  }

  static Vector<ValueType> FromArguments(Dart_NativeArguments args,
                                          int index,
                                          Dart_Handle& exception,
                                          bool auto_scope = true) {
    // TODO(abarth): What should we do with auto_scope?
    return FromDart(Dart_GetNativeArgument(args, index));
  }
};

////////////////////////////////////////////////////////////////////////////////
// DartValue

template <>
struct DartConverter<DartValue*> {
  static Dart_Handle ToDart(DartState* state, DartValue* val) {
    return val->dart_value();
  }

  static void SetReturnValue(Dart_NativeArguments args, DartValue* val) {
    Dart_SetReturnValue(args, val->dart_value());
  }

  static PassRefPtr<DartValue> FromDart(Dart_Handle handle) {
    return DartValue::Create(DartState::Current(), handle);
  }

  static PassRefPtr<DartValue> FromArguments(Dart_NativeArguments args,
                                             int index,
                                             Dart_Handle& exception,
                                             bool auto_scope = true) {
    // TODO(abarth): What should we do with auto_scope?
    return FromDart(Dart_GetNativeArgument(args, index));
  }
};

////////////////////////////////////////////////////////////////////////////////
// Convience wrappers for commonly used conversions

inline Dart_Handle StringToDart(DartState* state, const String& val) {
  return DartConverter<String>::ToDart(state, val);
}

inline Dart_Handle StringToDart(DartState* state, const AtomicString& val) {
  return DartConverter<AtomicString>::ToDart(state, val);
}

inline String StringFromDart(Dart_Handle handle) {
  return DartConverter<String>::FromDart(handle);
}

////////////////////////////////////////////////////////////////////////////////
// Convience wrappers using type inference for ease of code generation

template <typename T>
inline Dart_Handle VectorToDart(const Vector<T>& val) {
  return DartConverter<Vector<T>>::ToDart(val);
}

template<typename T>
Dart_Handle ToDart(const T& object) {
  return DartConverter<T>::ToDart(object);
}

template<typename T>
struct DartConverter<RefPtr<T>> {
  static Dart_Handle ToDart(RefPtr<T> val) {
    return DartConverter<T*>::ToDart(val.get());
  }

  static RefPtr<T> FromDart(Dart_Handle handle) {
    return DartConverter<T*>::FromDart(handle);
  }
};

#endif  // TONIC_DART_CONVERTER_WTF_H_
