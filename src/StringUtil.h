// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 04 2019

#ifndef ANT_STRING_UTIL_H
#define ANT_STRING_UTIL_H

#include <ctype.h>
#include <stdarg.h>   // For va_list
#include <stddef.h>
#include <stdint.h>

#include <initializer_list>
#include <string>
#include <vector>

#include "CompilerSpecific.h"
#include "StringPiece.h"


namespace annety {
// Contains the set of characters representing whitespace in the corresponding
// encoding. Null-terminated. The ASCII versions are the whitespaces as defined
// by HTML5, and don't include control characters.
extern const char kWhitespaceASCII[];

// Trim----------------------------------------------------

enum TrimPositions {
	TRIM_NONE     = 0,
	TRIM_LEADING  = 1 << 0,
	TRIM_TRAILING = 1 << 1,
	TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};

// Removes characters in |trim_chars| from the beginning and end of |input|.
bool trim_string(const std::string& input,
				 StringPiece trim_chars,
				 std::string* output);

// StringPiece versions of the above. The returned pieces refer to the original
// buffer.
StringPiece trim_string(StringPiece input,
						StringPiece trim_chars,
						TrimPositions positions);

// Trims any whitespace from either end of the input string.
TrimPositions trim_whitespace(const std::string& input,
							  TrimPositions positions,
							  std::string* output);
StringPiece trim_whitespace(StringPiece input,
							TrimPositions positions);

// Join------------------------------------------------------

// Does the opposite of SplitString()/SplitStringPiece(). Joins a vector or list
// of strings into a single string, inserting |separator| (which may be empty)
// in between all elements.
//
// If possible, callers should build a vector of StringPieces and use the
// StringPiece variant, so that they do not create unnecessary copies of
// strings. For example, instead of using SplitString, modifying the vector,
// then using JoinString, use SplitStringPiece followed by JoinString so that no
// copies of those strings are created until the final join operation.
//
// Use StrCat() if you don't need a separator.
std::string join_string(const std::vector<std::string>& parts,
						StringPiece separator);

std::string join_string(const std::vector<StringPiece>& parts,
						StringPiece separator);

// Explicit initializer_list overloads are required to break ambiguity when used
// with a literal initializer list (otherwise the compiler would not be able to
// decide between the string and StringPiece overloads).
std::string join_string(std::initializer_list<StringPiece> parts,
						StringPiece separator);

// Remove/Replace ----------------------------------------------------

// Removes characters in |remove_chars| from anywhere in |input|.  Returns true
// if any characters were removed.  |remove_chars| must be null-terminated.
// NOTE: Safe to use the same variable for both |input| and |output|.
bool remove_chars(const std::string& input,
				  StringPiece remove_chars,
				  std::string* output);

// Replaces characters in |replace_chars| from anywhere in |input| with
// |replace_with|.  Each character in |replace_chars| will be replaced with
// the |replace_with| string.  Returns true if any characters were replaced.
// |replace_chars| must be null-terminated.
// NOTE: Safe to use the same variable for both |input| and |output|.
bool replace_chars(const std::string& input,
				   StringPiece replace_chars,
				   const std::string& replace_with,
				   std::string* output);

// Starting at |start_offset| (usually 0), replace the first instance of
// |find_this| with |replace_with|.
void replace_first_substring_after_offset(std::string* str,
									  size_t start_offset,
									  StringPiece find_this,
									  StringPiece replace_with);

// Starting at |start_offset| (usually 0), look through |str| and replace all
// instances of |find_this| with |replace_with|.
//
// This does entire substrings; use std::replace in <algorithm> for single
// characters,
// Example:
//   std::replace(str.begin(), str.end(), 'a', 'b');
void replace_substrings_after_offset(std::string* str,
									 size_t start_offset,
									 StringPiece find_this,
									 StringPiece replace_with);

}	// namespace annety

#endif	// ANT_STRING_UTIL_H
