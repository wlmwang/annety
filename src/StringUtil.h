// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 04 2019

#ifndef ANT_STRING_UTIL_H
#define ANT_STRING_UTIL_H

#include "CompilerSpecific.h"	// PRINTF_FORMAT
#include "StringPiece.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>	// strdup
#include <stdio.h>	// vsnprintf

#include <initializer_list>
#include <string>
#include <vector>

namespace annety
{
namespace strings
{
// Misc----------------------------------------------------

// Chromium code style is to not use malloc'd strings; this is only for use
// for interaction with APIs that require it.
inline char* strdup(const char* str)
{
	return ::strdup(str);
}

inline int vsnprintf(char* buffer, size_t size,
					 const char* format, va_list arguments)
{
	return ::vsnprintf(buffer, size, format, arguments);
}

// Some of these implementations need to be inlined.

// We separate the declaration from the implementation of this inline
// function just so the PRINTF_FORMAT works.
inline int snprintf(char* buffer,
					size_t size,
					_Printf_format_string_ const char* format,
					...) PRINTF_FORMAT(3, 4);
inline int snprintf(char* buffer,
					size_t size,
					_Printf_format_string_ const char* format,
					...)
{
	va_list arguments;
	::va_start(arguments, format);
	int result = vsnprintf(buffer, size, format, arguments);
	::va_end(arguments);
	return result;
}

// BSD-style safe and consistent string copy functions.
// Copies |src| to |dst|, where |dst_size| is the total allocated size of |dst|.
// Copies at most |dst_size|-1 characters, and always NULL terminates |dst|, as
// long as |dst_size| is not 0.  Returns the length of |src| in characters.
// If the return value is >= dst_size, then the output was truncated.
// NOTE: All sizes are in number of characters, NOT in bytes.
size_t strlcpy(char* dst, const char* src, size_t dst_size);

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
inline char to_lower(char c)
{
	return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
inline char to_upper(char c)
{
	return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}

// Converts the given string to it's ASCII-lowercase equivalent.
std::string to_lower(StringPiece str);

// Converts the given string to it's ASCII-uppercase equivalent.
std::string to_upper(StringPiece str);

// Compare----------------------------------------------------

// Functor for case-insensitive ASCII comparisons for STL algorithms like
// std::search.
//
// Note that a full Unicode version of this functor is not possible to write
// because case mappings might change the number of characters, depend on
// context (combining accents), and require handling UTF-16. If you need
// proper Unicode support, use wutil::i18n::ToLower/FoldCase and then just
// use a normal operator== on the result.
template<typename Char> struct CaseInsensitiveCompare
{
public:
	bool operator()(Char x, Char y) const {
		return to_lower(x) == to_lower(y);
	}
};

// Like strcasecmp for case-insensitive ASCII characters only. Returns:
//   -1  (a < b)
//    0  (a == b)
//    1  (a > b)
// (unlike strcasecmp which can return values greater or less than 1/-1). For
// full Unicode support, use wutil::i18n::ToLower or wutil::i18h::FoldCase
// and then just call the normal string operators on the result.
int compare_case_insensitive(StringPiece a, StringPiece b);

// Equality for ASCII case-insensitive comparisons. For full Unicode support,
// use wutil::i18n::ToLower or wutil::i18h::FoldCase and then compare with either
// == or !=.
bool equals_case_insensitive(StringPiece a, StringPiece b);


// Returns true if |input| is empty or contains only characters found in
// |characters|.
bool contains_only_chars(StringPiece input, StringPiece characters);

// Compare the lower-case form of the given string against the given
// previously-lower-cased ASCII string (typically a constant).
bool lower_case_equals(StringPiece str,
					   StringPiece lowecase_ascii);

// Indicates case sensitivity of comparisons. Only ASCII case insensitivity
// is supported. Full Unicode case-insensitive conversions would need to go in
// wUtil/i18n so it can use ICU.
//
// If you need to do Unicode-aware case-insensitive StartsWith/EndsWith, it's
// best to call wutil::i18n::ToLower() or wutil::i18n::FoldCase() (see
// wUtil/i18n/case_conversion.h for usage advice) on the arguments, and then use
// the results to a case-sensitive comparison.
enum class CompareCase
{
	SENSITIVE,
	INSENSITIVE,
};

bool starts_with(StringPiece str,
				 StringPiece search_for,
				 CompareCase case_sensitivity);

bool ends_with(StringPiece str,
			   StringPiece search_for,
			   CompareCase case_sensitivity);

// Determines the type of ASCII character, independent of locale (the C
// library versions will change based on locale).
template <typename Char>
inline bool is_ascii_whitespace(Char c)
{
	return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
template <typename Char>
inline bool is_ascii_alpha(Char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
template <typename Char>
inline bool is_ascii_upper(Char c)
{
	return c >= 'A' && c <= 'Z';
}
template <typename Char>
inline bool is_ascii_lower(Char c)
{
	return c >= 'a' && c <= 'z';
}
template <typename Char>
inline bool is_ascii_digit(Char c)
{
	return c >= '0' && c <= '9';
}

template <typename Char>
inline bool is_hex_digit(Char c)
{
	return (c >= '0' && c <= '9') ||
		   (c >= 'A' && c <= 'F') ||
		   (c >= 'a' && c <= 'f');
}

// Contains the set of characters representing whitespace in the corresponding
// encoding. Null-terminated. The ASCII versions are the whitespaces as defined
// by HTML5, and don't include control characters.
extern const char kWhitespace[];

// Trim----------------------------------------------------

enum TrimPositions
{
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

// Reserves enough memory in |str| to accommodate |length_with_null| characters,
// sets the size of |str| to |length_with_null - 1| characters, and returns a
// pointer to the underlying contiguous array of characters.  This is typically
// used when calling a function that writes results into a character array, but
// the caller wants the data to be managed by a string-like object.  It is
// convenient in that is can be used inline in the call, and fast in that it
// avoids copying the results of the call from a char* into a string.
//
// |length_with_null| must be at least 2, since otherwise the underlying string
// would have size 0, and trying to access &((*str)[0]) in that case can result
// in a number of problems.
//
// Internally, this takes linear time because the resize() call 0-fills the
// underlying array for potentially all
// (|length_with_null - 1| * sizeof(string_type::value_type)) bytes.  Ideally we
// could avoid this aspect of the resize() call, as we expect the caller to
// immediately write over this memory, but there is no other way to set the size
// of the string, and not doing that will mean people who access |str| rather
// than str.c_str() will get back a string of whatever size |str| had on entry
// to this function (probably 0).
char* write_into(std::string* str, size_t length_with_null);

// Join----------------------------------------------------

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

// Replace----------------------------------------------------

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

}	// namespace strings
}	// namespace annety

#endif	// ANT_STRING_UTIL_H
