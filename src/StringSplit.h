// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 04 2019

#ifndef ANT_STRING_SPLIT_H
#define ANT_STRING_SPLIT_H

#include "CompilerSpecific.h"
#include "StringPiece.h"

#include <string>
#include <utility>
#include <vector>

namespace annety {

enum WhitespaceHandling {
	KEEP_WHITESPACE,
	TRIM_WHITESPACE,
};

enum SplitResult {
	// Strictly return all results.
	//
	// If the input is ",," and the separator is ',' this will return a
	// vector of three empty strings.
	SPLIT_WANT_ALL,

	// Only nonempty results will be added to the results. Multiple separators
	// will be coalesced. Separators at the beginning and end of the input will
	// be ignored. With TRIM_WHITESPACE, whitespace-only results will be dropped.
	//
	// If the input is ",," and the separator is ',', this will return an empty
	// vector.
	SPLIT_WANT_NONEMPTY,
};

// Split the given string on ANY of the given separators, returning copies of
// the result.
//
// To split on either commas or semicolons, keeping all whitespace:
//
//   std::vector<std::string> tokens = SplitString(
//       input, ",;", KEEP_WHITESPACE, SPLIT_WANT_ALL);
std::vector<std::string> split_string(StringPiece input,
									  StringPiece separators,
									  WhitespaceHandling whitespace,
									  SplitResult result_type);

// Like SplitString above except it returns a vector of StringPieces which
// reference the original buffer without copying. Although you have to be
// careful to keep the original string unmodified, this provides an efficient
// way to iterate through tokens in a string.
//
// To iterate through all whitespace-separated tokens in an input string:
//
//   for (const auto& cur :
//        SplitStringPiece(input, kWhitespace,
//                               KEEP_WHITESPACE,
//                               SPLIT_WANT_NONEMPTY)) {
//     ...
std::vector<StringPiece> split_string_piece(StringPiece input,
											StringPiece separators,
											WhitespaceHandling whitespace,
											SplitResult result_type);


using StringPairs = std::vector<std::pair<std::string, std::string>>;

// Splits |line| into key value pairs according to the given delimiters and
// removes whitespace leading each key and trailing each value. Returns true
// only if each pair has a non-empty key and value. |key_value_pairs| will
// include ("","") pairs for entries without |key_value_delimiter|.
bool split_string_into_key_value_pairs(StringPiece input,
									   char key_value_delimiter,
									   char key_value_pair_delimiter,
									   StringPairs* key_value_pairs);

// Similar to SplitString, but use a substring delimiter instead of a list of
// characters that are all possible delimiters.
std::vector<std::string> split_string_using_substr(StringPiece input,
												   StringPiece delimiter,
												   WhitespaceHandling whitespace,
												   SplitResult result_type);

// Like SplitStringUsingSubstr above except it returns a vector of StringPieces
// which reference the original buffer without copying. Although you have to be
// careful to keep the original string unmodified, this provides an efficient
// way to iterate through tokens in a string.
//
// To iterate through all newline-separated tokens in an input string:
//
//   for (const auto& cur :
//        SplitStringUsingSubstr(input, "\r\n",
//                                     KEEP_WHITESPACE,
//                                     SPLIT_WANT_NONEMPTY)) {
//     ...
std::vector<StringPiece> split_string_piece_using_substr(StringPiece input,
														 StringPiece delimiter,
														 WhitespaceHandling whitespace,
														 SplitResult result_type);

}	// namespace annety

#endif	// ANT_STRING_SPLIT_H
