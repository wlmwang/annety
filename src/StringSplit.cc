// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 04 2019

#include "strings/StringSplit.h"
#include "strings/StringUtil.h"
#include "Logging.h"

#include <string>
#include <stddef.h>

namespace annety
{
namespace
{
// piece_to_output_type converts a StringPiece as needed to a given output type,
// which is either the same type of StringPiece (a NOP) or the corresponding
// non-piece string type.
//
// The default converter is a NOP, it works when the OutputType is the
// correct StringPiece.
template<typename OutputType>
OutputType piece_to_output_type(StringPiece piece)
{
	return piece;
}
template<>  // Convert StringPiece to std::string
std::string piece_to_output_type<std::string>(StringPiece piece)
{
	return piece.as_string();
}

// Returns the ASCII whitespace.
StringPiece whitespace_for_type()
{
	return kWhitespace;
}

// Optimize the single-character case to call find() on the string instead,
// since this is the common case and can be made faster. This could have been
// done with template specialization too, but would have been less clear.
//
// There is no corresponding FindFirstNotOf because StringPiece already
// implements these different versions that do the optimized searching.
size_t find_first_of(StringPiece piece, char c, size_t pos)
{
	return piece.find(c, pos);
}
size_t find_first_of(StringPiece piece, StringPiece one_of, size_t pos)
{
	return piece.find_first_of(one_of, pos);
}

// General string splitter template. Can take 8- or 16-bit input, can produce
// the corresponding string or StringPiece output, and can take single- or
// multiple-character delimiters.
//
// DelimiterType is either a character (Str::value_type) or a string piece of
// multiple characters (BasicStringPiece<Str>). StringPiece has a version of
// find for both of these cases, and the single-character version is the most
// common and can be implemented faster, which is why this is a template.
template<typename OutputStringType, typename DelimiterType>
static std::vector<OutputStringType> split_string_T(StringPiece str,
													DelimiterType delimiter,
													WhitespaceHandling whitespace,
													SplitResult result_type)
{
	std::vector<OutputStringType> result;
	if (str.empty()) {
		return result;
	}

	size_t start = 0;
	while (start != StringPiece::npos) {
		size_t end = find_first_of(str, delimiter, start);

		StringPiece piece;
		if (end == StringPiece::npos) {
			piece = str.substr(start);
			start = StringPiece::npos;
		} else {
			piece = str.substr(start, end - start);
			start = end + 1;
		}

		if (whitespace == TRIM_WHITESPACE) {
			piece = trim_string(piece, whitespace_for_type(), TRIM_ALL);
		}

		if (result_type == SPLIT_WANT_ALL || !piece.empty()) {
			result.push_back(piece_to_output_type<OutputStringType>(piece));
		}
	}
	return result;
}

bool append_string_key_value(StringPiece input,
							 char delimiter,
							 StringPairs* result)
{
	// Always append a new item regardless of success (it might be empty). The
	// below code will copy the strings directly into the result pair.
	result->resize(result->size() + 1);
	auto& result_pair = result->back();

	// Find the delimiter.
	size_t end_key_pos = input.find_first_of(delimiter);
	if (end_key_pos == std::string::npos) {
		LOG(WARNING) << "cannot find delimiter in: " << input;
		return false;    // No delimiter.
	}
	input.substr(0, end_key_pos).copy_to_string(&result_pair.first);

	// Find the value string.
	StringPiece remains = input.substr(end_key_pos, input.size() - end_key_pos);
	size_t begin_value_pos = remains.find_first_not_of(delimiter);
	if (begin_value_pos == StringPiece::npos) {
		LOG(WARNING) << "cannot parse value from input: " << input;
		return false;   // No value.
	}
	remains.substr(begin_value_pos, remains.size() - begin_value_pos)
		.copy_to_string(&result_pair.second);

	return true;
}

template <typename OutputStringType>
void split_string_using_substr_T(StringPiece input,
							 StringPiece delimiter,
							 WhitespaceHandling whitespace,
							 SplitResult result_type,
							 std::vector<OutputStringType>* result) 
{
	using size_type = typename StringPiece::size_type;

	result->clear();
	for (size_type begin_index = 0, end_index = 0; 
		 end_index != StringPiece::npos;
		 begin_index = end_index + delimiter.size())
	{
		end_index = input.find(delimiter, begin_index);
		StringPiece term = end_index == StringPiece::npos
		? input.substr(begin_index)
		: input.substr(begin_index, end_index - begin_index);

		if (whitespace == TRIM_WHITESPACE) {
			term = trim_string(term, whitespace_for_type(), TRIM_ALL);
		}

		if (result_type == SPLIT_WANT_ALL || !term.empty()) {
			result->push_back(piece_to_output_type<OutputStringType>(term));
		}
	}
}

}	// namespace anonymous


std::vector<std::string> split_string(StringPiece input,
									  StringPiece separators,
									  WhitespaceHandling whitespace,
									  SplitResult result_type)
{
	if (separators.size() == 1) {
		return split_string_T<std::string, char>(
			input, separators[0], whitespace, result_type);
	}
	return split_string_T<std::string, StringPiece>(
		input, separators, whitespace, result_type);
}

std::vector<StringPiece> split_string_piece(StringPiece input,
											StringPiece separators,
											WhitespaceHandling whitespace,
											SplitResult result_type)
{
	if (separators.size() == 1) {
		return split_string_T<StringPiece, char>(
			input, separators[0], whitespace, result_type);
	}
	return split_string_T<StringPiece, StringPiece>(
		input, separators, whitespace, result_type);
}

bool split_string_into_key_value_pairs(StringPiece input,
									   char key_value_delimiter,
									   char key_value_pair_delimiter,
									   StringPairs* key_value_pairs)
{
	key_value_pairs->clear();

	std::vector<StringPiece> pairs = split_string_piece(
			input, std::string(1, key_value_pair_delimiter),
			TRIM_WHITESPACE, SPLIT_WANT_NONEMPTY);
	
	key_value_pairs->reserve(pairs.size());

	bool success = true;
	for (const StringPiece& pair : pairs) {
		if (!append_string_key_value(pair, key_value_delimiter, key_value_pairs)) {
			// Don't return here, to allow for pairs without associated
			// value or key; just record that the split failed.
			success = false;
		}
	}
	return success;
}

std::vector<std::string> split_string_using_substr(StringPiece input,
												   StringPiece delimiter,
												   WhitespaceHandling whitespace,
												   SplitResult result_type)
{
	std::vector<std::string> result;
	split_string_using_substr_T(input, delimiter, whitespace, result_type, &result);
	return result;
}

std::vector<StringPiece> split_string_piece_using_substr(StringPiece input,
														 StringPiece delimiter,
														 WhitespaceHandling whitespace,
														 SplitResult result_type)
{
	std::vector<StringPiece> result;
	split_string_using_substr_T(input, delimiter, whitespace, result_type, &result);
	return result;
}

}	// namespace annety
