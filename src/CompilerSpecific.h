// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#ifndef ANT_COMPILER_SPECIFIC_H_
#define ANT_COMPILER_SPECIFIC_H_

#include "BuildConfig.h"

// Annotate a variable indicating it's ok if the variable is not used.
// e.g:
// 		int x = 1; ALLOW_UNUSED_LOCAL(x);
#define ALLOW_UNUSED_LOCAL(x) (void)x

// Annotate a typedef or function indicating it's ok if it's not used.
// e.g:
// 		typedef Foo Bar ALLOW_UNUSED_TYPE;
#if defined(COMPILER_GCC) || defined(__clang__)
#define ALLOW_UNUSED_TYPE __attribute__((unused))
#else
#define ALLOW_UNUSED_TYPE
#endif

// Annotate a function indicating the caller must examine the return value.
// e.g:
//   int foo() WARN_UNUSED_RESULT;
#undef WARN_UNUSED_RESULT
#if defined(COMPILER_GCC) || defined(__clang__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

// Used to explicitly mark the return value of a function as unused. If you are
// really sure you don't want to do anything with the return value of a function
// that has been marked WARN_UNUSED_RESULT, wrap it with this. 
// e.g:
//   std::unique_ptr<MyType> my_var = ...;
//   if (TakeOwnership(my_var.get()) == SUCCESS)
//     ignore_result(my_var.release());
namespace annety {
template<typename T>
inline void ignore_result(const T&) {}
}	// namespace annety
#define IGNORE_UNUSED_RESULT(x) annety::ignore_result(x)

// Tell the compiler a function is using a printf-style format string.
// |format_param| is the one-based index of the format string parameter;
// |dots_param| is the one-based index of the "..." parameter.
// For v*printf functions (which take a va_list), pass 0 for dots_param.
// (This is undocumented but matches what the system C headers do.)
#define _Printf_format_string_

#if defined(COMPILER_GCC) || defined(__clang__)
#define PRINTF_FORMAT(format_param, dots_param) \
	__attribute__((format(printf, format_param, dots_param)))
#else
#define PRINTF_FORMAT(format_param, dots_param)
#endif

// Macro for hinting that an expression is likely to be false.
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

// Concatenate numbers in c/c++ macros to a token.
#define MACROS_CONCAT(a, b) a##b

// Convert symbol to string
#define MACROS_STRING(a) #a

// --------------------------------------------------------------------------
// Put this in the declarations for a class to be uncopyable.
#define DISALLOW_COPY(TypeName) TypeName(const TypeName&)=delete

// Put this in the declarations for a class to be unassignable.
#define DISALLOW_ASSIGN(TypeName) TypeName& operator=(const TypeName&)=delete

// Put this in the declarations for a class to be uncopyable and unassignable.
#define DISALLOW_COPY_AND_ASSIGN(TypeName)	\
	DISALLOW_COPY(TypeName);				\
	DISALLOW_ASSIGN(TypeName)

// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
// This is especially useful for classes containing only static methods.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)	\
	TypeName() = delete;							\
	DISALLOW_COPY_AND_ASSIGN(TypeName)

#endif	// ANT_COMPILER_SPECIFIC_H_
