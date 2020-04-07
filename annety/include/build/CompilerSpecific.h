// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#ifndef ANT_BUILD_COMPILER_SPECIFIC_H_
#define ANT_BUILD_COMPILER_SPECIFIC_H_

#include "build/BuildConfig.h"

// Annotate a variable indicating it's ok if the variable is not used.
// Example:
// int x = 1; ALLOW_UNUSED_LOCAL(x);
#define ALLOW_UNUSED_LOCAL(x) (void)x

// Annotate a typedef or function indicating it's ok if it's not used.
// Example:
// typedef Foo Bar ALLOW_UNUSED_TYPE;
#if defined(COMPILER_GCC) || defined(__clang__)
#define ALLOW_UNUSED_TYPE __attribute__((unused))
#else
#define ALLOW_UNUSED_TYPE
#endif

// Annotate a function indicating the caller must examine the return value.
// Example:
// int foo() WARN_UNUSED_RESULT;
#undef WARN_UNUSED_RESULT
#if defined(COMPILER_GCC) || defined(__clang__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

// Used to explicitly mark the return value of a function as unused. If you are
// really sure you don't want to do anything with the return value of a function
// that has been marked WARN_UNUSED_RESULT, wrap it with this. 
// Example:
// std::unique_ptr<MyType> my_var = ...;
// if (TakeOwnership(my_var.get()) == SUCCESS) {
//		ALLOW_UNUSED_RESULT(my_var.release());
// }
namespace annety {
namespace internal {
template<typename T>
inline void ignore_result(const T&) {}
}	// namespace internal
}	// namespace annety
#define ALLOW_UNUSED_RESULT(x) annety::internal::ignore_result(x)

// Tell the compiler a function is using a printf-style format string.
// |format_param| is the one-based index of the format string parameter;
// |dots_param| is the one-based index of the "..." parameter.
// For v*printf functions (which take a va_list), pass 0 for dots_param.
// (This is undocumented but matches what the system C headers do.)
#define _Printf_format_string_
// ---
#if defined(COMPILER_GCC) || defined(__clang__)
#define PRINTF_FORMAT(format_param, dots_param) \
	__attribute__((format(printf, format_param, dots_param)))
#else
#define PRINTF_FORMAT(format_param, dots_param)
#endif

// Macro for hinting that an expression is likely to be false.
// see: https://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Other-Builtins.html
// Example:
// if (UNLIKELY(data == 0)) {
//		//... your code ...
// }
#if !defined(UNLIKELY)
#if defined(COMPILER_GCC) || defined(__clang__)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define UNLIKELY(x) (x)
#endif	// defined(COMPILER_GCC)
#endif	// !defined(UNLIKELY)
// ---
#if !defined(LIKELY)
#if defined(COMPILER_GCC) || defined(__clang__)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define LIKELY(x) (x)
#endif	// defined(COMPILER_GCC)
#endif	// !defined(LIKELY)

#endif	// ANT_BUILD_COMPILER_SPECIFIC_H_
