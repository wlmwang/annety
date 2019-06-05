// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 04 2019

#include "StringUtil.h"
#include "Logging.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <limits>
#include <vector>
#include <iostream>

namespace annety {
const char kWhitespaceASCII[] = {
	0x09,	// CHARACTER TABULATION
	0x0A,	// LINE FEED (LF)
	0x0B,	// LINE TABULATION
	0x0C,	// FORM FEED (FF)
	0x0D,	// CARRIAGE RETURN (CR)
	0x20,	// SPACE
	0
};

// Misc----------------------------------------------------

// The following code is compatible with the OpenBSD lcpy interface.  See:
//   http://www.gratisoft.us/todd/papers/strlcpy.html
//   ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/{wcs,str}lcpy.c

namespace {
template <typename CHAR>
size_t lcpyT(CHAR* dst, const CHAR* src, size_t dst_size) {
	for (size_t i = 0; i < dst_size; ++i) {
		// We hit and copied the terminating NULL.
		if ((dst[i] = src[i]) == 0) {
			return i;
		}
	}

	// We were left off at dst_size.  We over copied 1 byte.  Null terminate.
	if (dst_size != 0) {
		dst[dst_size - 1] = 0;
	}

	// Count the rest of the |src|, and return it's length in characters.
	while (src[dst_size]) {
		++dst_size;
	}
	return dst_size;
}

}	// namespace anonymous

size_t strlcpy(char* dst, const char* src, size_t dst_size) {
	return lcpyT<char>(dst, src, dst_size);
}

namespace {
std::string to_lower_T(StringPiece str) {
	std::string ret;
	ret.reserve(str.size());
	for (size_t i = 0; i < str.size(); i++) {
		ret.push_back(to_lower(str[i]));
	}
	return ret;
}

std::string to_upper_T(StringPiece str) {
	std::string ret;
	ret.reserve(str.size());
	for (size_t i = 0; i < str.size(); i++) {
		ret.push_back(to_upper(str[i]));
	}
	return ret;
}

}	// namespace anonymous

std::string to_lower(StringPiece str) {
	return to_lower_T(str);
}

std::string to_upper(StringPiece str) {
	return to_upper_T(str);
}

// Compare----------------------------------------------------

namespace {
int compare_case_insensitive_T(StringPiece a,
                               StringPiece b)
{
	// Find the first characters that aren't equal and compare them.  If the end
	// of one of the strings is found before a nonequal character, the lengths
	// of the strings are compared.
	size_t i = 0;
	while (i < a.length() && i < b.length()) {
		typename std::string::value_type lower_a = to_lower(a[i]);
		typename std::string::value_type lower_b = to_lower(b[i]);
		if (lower_a < lower_b) {
			return -1;
		}
		if (lower_a > lower_b) {
			return 1;
		}	
		i++;
	}

	// End of one string hit before finding a different character. Expect the
	// common case to be "strings equal" at this point so check that first.
	if (a.length() == b.length()) {
		return 0;
	}
	if (a.length() < b.length()) {
		return -1;
	}	
	return 1;
}

}	// namespace anonymous

int compare_case_insensitive(StringPiece a, StringPiece b) {
	return compare_case_insensitive_T(a, b);
}

bool equals_case_insensitive(StringPiece a, StringPiece b) {
	if (a.length() != b.length()) {
		return false;
	}
	return compare_case_insensitive_T(a, b) == 0;
}

bool contains_only_chars(StringPiece input, StringPiece characters) {
	return input.find_first_not_of(characters) == StringPiece::npos;
}

// Implementation note: Normally this function will be called with a hardcoded
// constant for the lowercase_ascii parameter. Constructing a StringPiece from
// a C constant requires running strlen, so the result will be two passes
// through the buffers, one to file the length of lowercase_ascii, and one to
// compare each letter.
//
// This function could have taken a const char* to avoid this and only do one
// pass through the string. But the strlen is faster than the case-insensitive
// compares and lets us early-exit in the case that the strings are different
// lengths (will often be the case for non-matches). So whether one approach or
// the other will be faster depends on the case.
//
// The hardcoded strings are typically very short so it doesn't matter, and the
// string piece gives additional flexibility for the caller (doesn't have to be
// null terminated) so we choose the StringPiece route.
namespace {
inline bool lower_case_equals_T(StringPiece str,
								StringPiece lowercase_ascii)
{
	if (str.size() != lowercase_ascii.size()) {
		return false;
	}
	for (size_t i = 0; i < str.size(); i++) {
		if (to_lower(str[i]) != lowercase_ascii[i]) {
			return false;
		}
	}
	return true;
}

}	// namespace anonymous

bool lower_case_equals(StringPiece str, StringPiece lowercase_ascii) {
	return lower_case_equals_T(str, lowercase_ascii);
}

namespace {
bool starts_with_T(StringPiece str,
				   StringPiece search_for,
				   CompareCase case_sensitivity)
{
	if (search_for.size() > str.size()) {
		return false;
	}

	StringPiece source = str.substr(0, search_for.size());

	switch (case_sensitivity) {
		case CompareCase::SENSITIVE:
			return source == search_for;

		case CompareCase::INSENSITIVE_ASCII:
			return std::equal(
					search_for.begin(), search_for.end(),
					source.begin(),
					CaseInsensitiveCompare<typename std::string::value_type>());

		default:
			NOTREACHED();
			return false;
	}
}

}	// namespace anonymous

bool starts_with(StringPiece str,
				 StringPiece search_for,
				 CompareCase case_sensitivity)
{
	return starts_with_T(str, search_for, case_sensitivity);
}

namespace {
bool ends_with_T(StringPiece str,
				 StringPiece search_for,
				 CompareCase case_sensitivity)
{
	if (search_for.size() > str.size()) {
		return false;
	}

	StringPiece source = str.substr(str.size() - search_for.size(),
									search_for.size());

	switch (case_sensitivity) {
		case CompareCase::SENSITIVE:
			return source == search_for;

		case CompareCase::INSENSITIVE_ASCII:
			return std::equal(
					source.begin(), source.end(),
					search_for.begin(),
					CaseInsensitiveCompare<typename std::string::value_type>());

		default:
			NOTREACHED();
			return false;
	}
}

}	// namespace anonymous

bool ends_with(StringPiece str,
			   StringPiece search_for,
			   CompareCase case_sensitivity)
{
	return ends_with_T(str, search_for, case_sensitivity);
}

// Trim----------------------------------------------------
namespace {
TrimPositions trim_string_T(const std::string& input,
							StringPiece trim_chars,
							TrimPositions positions,
							std::string* output)
{
	// Find the edges of leading/trailing whitespace as desired. Need to use
	// a StringPiece version of input to be able to call find* on it with the
	// StringPiece version of trim_chars (normally the trim_chars will be a
	// constant so avoid making a copy).
	StringPiece input_piece(input);
	const size_t last_char = input.length() - 1;
	const size_t first_good_char = (positions & TRIM_LEADING) ?
					input_piece.find_first_not_of(trim_chars) : 0;
	const size_t last_good_char = (positions & TRIM_TRAILING) ?
					input_piece.find_last_not_of(trim_chars) : last_char;

	// When the string was all trimmed, report that we stripped off characters
	// from whichever position the caller was interested in. For empty input, we
	// stripped no characters, but we still need to clear |output|.
	if (input.empty() ||
		(first_good_char == std::string::npos) || 
		(last_good_char == std::string::npos))
	{
		bool input_was_empty = input.empty();  // in case output == &input
		output->clear();
		return input_was_empty ? TRIM_NONE : positions;
	}

	// Trim.
	*output = input.substr(first_good_char, 
						   last_good_char - first_good_char + 1);

	// Return where we trimmed from.
	return static_cast<TrimPositions>(
		((first_good_char == 0) ? TRIM_NONE : TRIM_LEADING) |
		((last_good_char == last_char) ? TRIM_NONE : TRIM_TRAILING));
}

StringPiece trim_string_piece_T(StringPiece input,
								StringPiece trim_chars,
								TrimPositions positions)
{
	size_t begin = (positions & TRIM_LEADING) ?
		input.find_first_not_of(trim_chars) : 0;
	size_t end = (positions & TRIM_TRAILING) ?
		input.find_last_not_of(trim_chars) + 1 : input.size();
		
	return input.substr(begin, end - begin);
}

}	// namespace anonymous

bool trim_string(const std::string& input,
				 StringPiece trim_chars,
				 std::string* output)
{
	return trim_string_T(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

StringPiece trim_string(StringPiece input,
						StringPiece trim_chars,
						TrimPositions positions)
{
	return trim_string_piece_T(input, trim_chars, positions);
}

TrimPositions trim_whitespace(const std::string& input,
							  TrimPositions positions,
							  std::string* output)
{
	return trim_string_T(input, StringPiece(kWhitespaceASCII), positions, output);
}

StringPiece trim_whitespace(StringPiece input, TrimPositions positions) {
	return trim_string_piece_T(input, StringPiece(kWhitespaceASCII), positions);
}

namespace {
inline typename std::string::value_type* write_into_T(std::string* str,
													  size_t length_with_null) {
	DCHECK_GT(length_with_null, 1u);
	str->reserve(length_with_null);
	str->resize(length_with_null - 1);
	return &((*str)[0]);
}

}	// namespace anonymous

char* write_into(std::string* str, size_t length_with_null) {
	return write_into_T(str, length_with_null);
}

// Join----------------------------------------------------

namespace {
// Overloaded function to append one string onto the end of another. Having a
// separate overload for |source| as both string and StringPiece allows for more
// efficient usage from functions templated to work with either type (avoiding a
// redundant call to the StringPiece constructor in both cases).
inline void append_to_string(std::string* target, const std::string& source) {
	target->append(source);
}
inline void append_to_string(std::string* target, const StringPiece& source) {
	source.append_to_string(target);
}

// Generic version for all JoinString overloads. |list_type| must be a sequence
// (std::vector or std::initializer_list) of strings/StringPieces (std::string,
// StringPiece).
template <typename list_type>
static std::string join_string_T(const list_type& parts,
								 StringPiece sep)
{
	if (parts.size() == 0) {
		return std::string();
	}

	// Pre-allocate the eventual size of the string. Start with the size of all of
	// the separators (note that this *assumes* parts.size() > 0).
	size_t total_size = (parts.size() - 1) * sep.size();
	for (const auto& part : parts) {
		total_size += part.size();
	}
  
	std::string result;
	result.reserve(total_size);

	auto iter = parts.begin();
	DCHECK(iter != parts.end());
	append_to_string(&result, *iter);
	++iter;

	for (; iter != parts.end(); ++iter) {
		sep.append_to_string(&result);
		// Using the overloaded append_to_string allows this template function to work
		// on both strings and StringPieces without creating an intermediate
		// StringPiece object.
		append_to_string(&result, *iter);
	}
	return result;
}

}	// namespace anonymous

std::string join_string(const std::vector<std::string>& parts,
						StringPiece separator)
{
	return join_string_T(parts, separator);
}

std::string join_string(const std::vector<StringPiece>& parts,
						StringPiece separator)
{
	return join_string_T(parts, separator);
}

std::string join_string(std::initializer_list<StringPiece> parts,
						StringPiece separator)
{
	return join_string_T(parts, separator);
}

// Replace----------------------------------------------------

namespace {
// A Matcher for DoReplaceMatchesAfterOffset() that matches substrings.
struct SubstringMatcher {
	StringPiece find_this;

	size_t find(const std::string& input, size_t pos) {
		return input.find(find_this.data(), pos, find_this.length());
	}
	size_t match_size() {
		return find_this.length();
	}
};

// A Matcher for DoReplaceMatchesAfterOffset() that matches single characters.
struct CharacterMatcher {
	StringPiece find_any_of_these;

	size_t find(const std::string& input, size_t pos) {
		return input.find_first_of(find_any_of_these.data(), pos,
					find_any_of_these.length());
	}
	size_t match_size() {
		return 1;
	}
};

enum class ReplaceType { REPLACE_ALL, REPLACE_FIRST };

// Runs in O(n) time in the length of |str|, and transforms the string without
// reallocating when possible. Returns |true| if any matches were found.
//
// This is parameterized on a |Matcher| traits type, so that it can be the
// implementation for both ReplaceChars() and ReplaceSubstringsAfterOffset().
template <class Matcher>
bool do_replace_matches_after_offset(std::string* str,
									 size_t initial_offset,
									 Matcher matcher,
									 StringPiece replace_with,
									 ReplaceType replace_type)
{
	using CharTraits = typename std::string::traits_type;

	const size_t find_length = matcher.match_size();
	if (!find_length) {
		return false;
	}

	// If the find string doesn't appear, there's nothing to do.
	size_t first_match = matcher.find(*str, initial_offset);
	if (first_match == std::string::npos) {
		return false;
	}

	// If we're only replacing one instance, there's no need to do anything
	// complicated.
	const size_t replace_length = replace_with.length();
	if (replace_type == ReplaceType::REPLACE_FIRST) {
		str->replace(first_match, find_length, replace_with.data(), replace_length);
		return true;
	}

	// If the find and replace strings are the same length, we can simply use
	// replace() on each instance, and finish the entire operation in O(n) time.
	if (find_length == replace_length) {
		auto* buffer = &((*str)[0]);
		for (size_t offset = first_match; offset != std::string::npos;
			 offset = matcher.find(*str, offset + replace_length))
		{
			CharTraits::copy(buffer + offset, replace_with.data(), replace_length);
		}
		return true;
	}

	// Since the find and replace strings aren't the same length, a loop like the
	// one above would be O(n^2) in the worst case, as replace() will shift the
	// entire remaining string each time. We need to be more clever to keep things
	// O(n).
	//
	// When the string is being shortened, it's possible to just shift the matches
	// down in one pass while finding, and truncate the length at the end of the
	// search.
	//
	// If the string is being lengthened, more work is required. The strategy used
	// here is to make two find() passes through the string. The first pass counts
	// the number of matches to determine the new size. The second pass will
	// either construct the new string into a new buffer (if the existing buffer
	// lacked capacity), or else -- if there is room -- create a region of scratch
	// space after |first_match| by shifting the tail of the string to a higher
	// index, and doing in-place moves from the tail to lower indices thereafter.
	size_t str_length = str->length();
	size_t expansion = 0;
	if (replace_length > find_length) {
		// This operation lengthens the string; determine the new length by counting
		// matches.
		const size_t expansion_per_match = (replace_length - find_length);
		size_t num_matches = 0;
		for (size_t match = first_match; match != std::string::npos;
			match = matcher.find(*str, match + find_length))
		{
			expansion += expansion_per_match;
			++num_matches;
		}
    	const size_t final_length = str_length + expansion;

		if (str->capacity() < final_length) {
			// If we'd have to allocate a new buffer to grow the string, build the
			// result directly into the new allocation via append().
			std::string src(str->get_allocator());
			str->swap(src);
			str->reserve(final_length);

			size_t pos = 0;
			for (size_t match = first_match;; match = matcher.find(src, pos)) {
				str->append(src, pos, match - pos);
				str->append(replace_with.data(), replace_length);
				pos = match + find_length;

				// A mid-loop test/break enables skipping the final Find() call; the
				// number of matches is known, so don't search past the last one.
				if (!--num_matches) {
					break;
				}
			}

			// Handle substring after the final match.
			str->append(src, pos, str_length - pos);
			return true;
		}

		// Prepare for the copy/move loop below -- expand the string to its final
		// size by shifting the data after the first match to the end of the resized
		// string.
		size_t shift_src = first_match + find_length;
		size_t shift_dst = shift_src + expansion;

		// Big |expansion| factors (relative to |str_length|) require padding up to
		// |shift_dst|.
		if (shift_dst > str_length) {
			str->resize(shift_dst);
		}

		str->replace(shift_dst, str_length - shift_src, *str, shift_src,
					 str_length - shift_src);
		str_length = final_length;
	}

	// We can alternate replacement and move operations. This won't overwrite the
	// unsearched region of the string so long as |write_offset| <= |read_offset|;
	// that condition is always satisfied because:
	//
	//   (a) If the string is being shortened, |expansion| is zero and
	//       |write_offset| grows slower than |read_offset|.
	//
	//   (b) If the string is being lengthened, |write_offset| grows faster than
	//       |read_offset|, but |expansion| is big enough so that |write_offset|
	//       will only catch up to |read_offset| at the point of the last match.
	auto* buffer = &((*str)[0]);
	size_t write_offset = first_match;
	size_t read_offset = first_match + expansion;
	do {
		if (replace_length) {
			CharTraits::copy(buffer + write_offset, replace_with.data(),
							 replace_length);
			write_offset += replace_length;
		}
		read_offset += find_length;

		// min() clamps std::string::npos (the largest unsigned value) to str_length.
		size_t match = std::min(matcher.find(*str, read_offset), str_length);

		size_t length = match - read_offset;
		if (length) {
			CharTraits::move(buffer + write_offset, buffer + read_offset, length);
			write_offset += length;
			read_offset += length;
		}
	} while (read_offset < str_length);

	// If we're shortening the string, truncate it now.
	str->resize(write_offset);
	return true;
}

bool replace_chars_T(const std::string& input,
					 StringPiece find_any_of_these,
					 StringPiece replace_with,
					 std::string* output)
{
	// Commonly, this is called with output and input being the same string; in
	// that case, this assignment is inexpensive.
	*output = input;

	return do_replace_matches_after_offset(
				output, 0, CharacterMatcher{find_any_of_these},
				replace_with, ReplaceType::REPLACE_ALL);
}

}	// namespace anonymous

bool remove_chars(const std::string& input,
                  StringPiece remove_chars,
                  std::string* output)
{
	return replace_chars_T(input, remove_chars, StringPiece(), output);
}

bool replace_chars(const std::string& input,
                   StringPiece replace_chars,
                   const std::string& replace_with,
                   std::string* output)
{
	return replace_chars_T(input, replace_chars, StringPiece(replace_with), output);
}

void replace_first_substring_after_offset(std::string* str,
										  size_t start_offset,
										  StringPiece find_this,
										  StringPiece replace_with)
{
	do_replace_matches_after_offset(str, start_offset,
			SubstringMatcher{find_this},
			replace_with, ReplaceType::REPLACE_FIRST);
}

void replace_substrings_after_offset(std::string* str,
									 size_t start_offset,
									 StringPiece find_this,
									 StringPiece replace_with)
{
	do_replace_matches_after_offset(str, start_offset,
			SubstringMatcher{find_this},
			replace_with, ReplaceType::REPLACE_ALL);
}

}	// namespace annety
