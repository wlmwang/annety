// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#ifndef ANT_STRINGS_STRING_PRINTF_H_
#define ANT_STRINGS_STRING_PRINTF_H_

#include "build/CompilerSpecific.h"	// _Printf_format_string_
									// PRINTF_FORMAT,WARN_UNUSED_RESULT

#include <string>
#include <stdarg.h>   // va_list

namespace annety
{
// Example:
// // StringPrintf
// string sf = string_printf("printf(%%3.1) %3.1f", 111.111);
// cout << sf << endl;
//
// cout << sstring_appendf(&sf, "*%s", "222.222") << endl;
// ...

// Return a C++ string given printf-like input.
std::string string_printf(_Printf_format_string_ const char* format, ...)
	PRINTF_FORMAT(1, 2) WARN_UNUSED_RESULT;

// Return a C++ string given vprintf-like input.
std::string string_printv(_Printf_format_string_ const char* format, va_list ap)
	PRINTF_FORMAT(1, 0) WARN_UNUSED_RESULT;

// Store result into a supplied string and return it.
const std::string& sstring_printf(std::string* dst,
	_Printf_format_string_ const char* format, ...) 
	PRINTF_FORMAT(2, 3);

const std::string& sstring_printv(std::string* dst, 
	_Printf_format_string_ const char* format, va_list ap)
	PRINTF_FORMAT(2, 0);

// Append result to a supplied string.
const std::string& sstring_appendf(std::string* dst,
	_Printf_format_string_ const char* format, ...) 
	PRINTF_FORMAT(2, 3);

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
const std::string& sstring_appendv(std::string* dst, 
	_Printf_format_string_ const char* format, va_list ap)
	PRINTF_FORMAT(2, 0);

// ::snprintf wrapper function, printf-like input
int sstring_printf(char* dst, size_t len,
	_Printf_format_string_ const char* format, ...)
	PRINTF_FORMAT(3, 4);

// ::vsnprintf wrapper function, vprintf-like input
int sstring_printv(char* dst, size_t len,
	_Printf_format_string_ const char* format, va_list ap)
	PRINTF_FORMAT(3, 0);

}  // namespace annety

#endif  // ANT_STRINGS_STRING_PRINTF_H_
