// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#ifndef ANT_MACROS_H_
#define ANT_MACROS_H_

#include "build/BuildConfig.h"

#include <stddef.h>

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

// C++14 implementation of C++17's std::size():
// http://en.cppreference.com/w/cpp/iterator/size
namespace annety {
namespace internal {
// Get Container(STL) count at run-time.
template <typename Container>
constexpr auto size(const Container& c) -> decltype(c.size()) {
	return c.size();
}
// Get array count at compile-time.
template <typename T, size_t N>
constexpr size_t size(const T (&array)[N]) noexcept {
	return N;
}
}	// namespace internal
}	// namespace annety
#define arraysize(x) annety::internal::size(x)

// Concatenate two symbol to a token.
#define MACROS_CONCAT(a, b) a##b

// Convert symbol to c-style string
#define MACROS_STRING(a) #a

// Put following code somewhere global to run it before main().
// DO NOT CREATE ANY THREAD BEFORE RUN main()!!!
// Example:
// BEFORE_MAIN_EXECUTOR()
// {
//		//... your code ...
// }
// You can write code:
//   * Use CHECK_*/LOG(...).
//   * Have multiple BEFORE_MAIN_EXECUTOR() in one scope.
#if defined(__cplusplus)
#define BEFORE_MAIN_EXECUTOR								\
namespace {													\
	struct MACROS_CONCAT(_GlobalInit, __LINE__) {			\
		MACROS_CONCAT(_GlobalInit, __LINE__)() { init(); }	\
		void init();										\
	} MACROS_CONCAT(g_global_init_dummy_, __LINE__);		\
}	/* namespace anonymous */								\
void MACROS_CONCAT(_GlobalInit, __LINE__)::init
#else
#define BEFORE_MAIN_EXECUTOR								\
	static void __attribute__((constructor))				\
	MACROS_CONCAT(g_global_init_dummy_, __LINE__)
#endif

#endif	// ANT_MACROS_H_
