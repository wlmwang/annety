// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: May 08 2019

#include "strings/StringPiece.h"
#include "Logging.h"

#include <algorithm>	// find,search
#include <ostream>
#include <limits.h>		// UCHAR_MAX
#include <stddef.h>

namespace annety
{
namespace {
// For each character in characters_wanted, sets the index corresponding
// to the ASCII code of that character to 1 in table.  This is used by
// the find_.*_of methods below to tell whether or not a character is in
// the lookup table in constant time.
// The argument `table' must be an array that is large enough to hold all
// the possible values of an unsigned char.  Thus it should be be declared
// as follows:
//   bool table[UCHAR_MAX + 1]
inline void build_lookup_table(const StringPiece& characters_wanted,
							   bool* table)
{
	const size_t length = characters_wanted.length();
	const char* const data = characters_wanted.data();
	for (size_t i = 0; i < length; ++i) {
		table[static_cast<unsigned char>(data[i])] = true;
	}
}

}	// namespace anonymous

namespace internal
{
void copy_to_string_T(const StringPiece& self, std::string* target)
{
	if (self.empty()) {
		target->clear();
	} else {
		target->assign(self.data(), self.size());
	}
}

void copy_to_string(const StringPiece& self, std::string* target)
{
	copy_to_string_T(self, target);
}

void append_to_string_T(const StringPiece& self, std::string* target)
{
	if (!self.empty()) {
		target->append(self.data(), self.size());
	}
}

void append_to_string(const StringPiece& self, std::string* target)
{
	append_to_string_T(self, target);
}

size_t copy_T(const StringPiece& self,
			  typename std::string::value_type* target,
			  size_t n, size_t pos)
{
	size_t ret = std::min(self.size() - pos, n);
	::memcpy(target, self.data() + pos, ret * sizeof(typename std::string::value_type));
	return ret;
}

size_t copy(const StringPiece& self, char* target, size_t n, size_t pos)
{
	return copy_T(self, target, n, pos);
}

size_t find_T(const StringPiece& self,
			  const StringPiece& s,
			  size_t pos)
{
	if (pos > self.size()) {
		return StringPiece::npos;
	}

	typename StringPiece::const_iterator result =
		std::search(self.begin() + pos, self.end(), s.begin(), s.end());
	const size_t xpos =
		static_cast<size_t>(result - self.begin());
	
	return xpos + s.size() <= self.size() ? xpos : StringPiece::npos;
}

size_t find(const StringPiece& self, const StringPiece& s, size_t pos)
{
	return find_T(self, s, pos);
}

size_t find_T(const StringPiece& self,
			  typename std::string::value_type c,
			  size_t pos)
{
	if (pos >= self.size()) {
		return StringPiece::npos;
	}

	typename StringPiece::const_iterator result =
		std::find(self.begin() + pos, self.end(), c);
	
	return result != self.end() ?
			static_cast<size_t>(result - self.begin()) : StringPiece::npos;
}

size_t find(const StringPiece& self, char c, size_t pos)
{
	return find_T(self, c, pos);
}

size_t rfind_T(const StringPiece& self,
			  const StringPiece& s,
			  size_t pos) 
{
	if (self.size() < s.size()) {
		return StringPiece::npos;
	}

	if (s.empty()) {
		return std::min(self.size(), pos);
	}

	typename StringPiece::const_iterator last =
		self.begin() + std::min(self.size() - s.size(), pos) + s.size();
	typename StringPiece::const_iterator result =
		std::find_end(self.begin(), last, s.begin(), s.end());

	return result != last ?
		static_cast<size_t>(result - self.begin()) : StringPiece::npos;
}

size_t rfind(const StringPiece& self, const StringPiece& s, size_t pos)
{
	return rfind_T(self, s, pos);
}

size_t rfind_T(const StringPiece& self,
			   typename std::string::value_type c,
			   size_t pos)
{
	if (self.size() == 0) {
		return StringPiece::npos;
	}

	for (size_t i = std::min(pos, self.size() - 1); ;
		 --i)
	{
		if (self.data()[i] == c) {
			return i;
		}
		if (i == 0) {
			break;
		} 
	}
  	return StringPiece::npos;
}

size_t rfind(const StringPiece& self, char c, size_t pos)
{
	return rfind_T(self, c, pos);
}

// 8-bit version using lookup table.
size_t find_first_of(const StringPiece& self,
					 const StringPiece& s,
					 size_t pos)
{
	if (self.size() == 0 || s.size() == 0) {
		return StringPiece::npos;
	}

	// Avoid the cost of BuildLookupTable() for a single-character search.
	if (s.size() == 1) {
		return find(self, s.data()[0], pos);
	}

	bool lookup[UCHAR_MAX + 1] = { false };
	build_lookup_table(s, lookup);
	for (size_t i = pos; i < self.size(); ++i) {
		if (lookup[static_cast<unsigned char>(self.data()[i])]) {
			return i;
		}
	}
	return StringPiece::npos;
}

// 8-bit version using lookup table.
size_t find_first_not_of(const StringPiece& self,
						 const StringPiece& s,
						 size_t pos)
{
	if (self.size() == 0) {
		return StringPiece::npos;
	}

	if (s.size() == 0) {
		return 0;
	}

	// Avoid the cost of BuildLookupTable() for a single-character search.
	if (s.size() == 1) {
		return find_first_not_of(self, s.data()[0], pos);
	}
    
	bool lookup[UCHAR_MAX + 1] = { false };
	build_lookup_table(s, lookup);
	for (size_t i = pos; i < self.size(); ++i) {
		if (!lookup[static_cast<unsigned char>(self.data()[i])]) {
			return i;
		}
	}
	return StringPiece::npos;
}

size_t find_first_not_of_T(const StringPiece& self,
						   typename std::string::value_type c,
						   size_t pos)
{
	if (self.size() == 0) {
		return StringPiece::npos;
	}

	for (; pos < self.size(); ++pos) {
		if (self.data()[pos] != c) {
			return pos;
		}
	}
	return StringPiece::npos;
}

size_t find_first_not_of(const StringPiece& self, char c, size_t pos)
{
	return find_first_not_of_T(self, c, pos);
}

// 8-bit version using lookup table.
size_t find_last_of(const StringPiece& self, const StringPiece& s, size_t pos)
{
	if (self.size() == 0 || s.size() == 0) {
		return StringPiece::npos;
	}

	// Avoid the cost of build_lookup_table() for a single-character search.
	if (s.size() == 1) {
		return rfind(self, s.data()[0], pos);
	}

	bool lookup[UCHAR_MAX + 1] = { false };
	build_lookup_table(s, lookup);
	for (size_t i = std::min(pos, self.size() - 1); ; --i) {
		if (lookup[static_cast<unsigned char>(self.data()[i])]) {
			return i;
		}
		if (i == 0) {
			break;
		}
	}
	return StringPiece::npos;
}

// 8-bit version using lookup table.
size_t find_last_not_of(const StringPiece& self,
						const StringPiece& s,
						size_t pos)
{
	if (self.size() == 0) {
		return StringPiece::npos;
	}

	size_t i = std::min(pos, self.size() - 1);
	if (s.size() == 0) {
		return i;
	}

	// Avoid the cost of build_lookup_table() for a single-character search.
	if (s.size() == 1) {
		return find_last_not_of(self, s.data()[0], pos);
	}
    
	bool lookup[UCHAR_MAX + 1] = { false };
	build_lookup_table(s, lookup);
	for (; ; --i) {
		if (!lookup[static_cast<unsigned char>(self.data()[i])]) {
			return i;
		}	
		if (i == 0) {
			break;
		}
	}
	return StringPiece::npos;
}

size_t find_last_not_of_T(const StringPiece& self,
						  typename std::string::value_type c,
						  size_t pos)
{
	if (self.size() == 0) {
		return StringPiece::npos;
	}

	for (size_t i = std::min(pos, self.size() - 1); ; --i) {
		if (self.data()[i] != c) {
			return i;
		}
		if (i == 0) {
			break;
		}
	}
	return StringPiece::npos;
}

size_t find_last_not_of(const StringPiece& self, char c, size_t pos)
{
	return find_last_not_of_T(self, c, pos);
}

StringPiece substr_T(const StringPiece& self,
					 size_t pos,
					 size_t n)
{
	if (pos > self.size()) {
		pos = self.size();
	}
	if (n > self.size() - pos) {
		n = self.size() - pos;
	}
	return StringPiece(self.data() + pos, n);
}

StringPiece substr(const StringPiece& self, size_t pos, size_t n)
{
	return substr_T(self, pos, n);
}

void assert_iterators_in_order(std::string::const_iterator begin,
							   std::string::const_iterator end)
{
	DCHECK(begin <= end) << "StringPiece iterators swapped or invalid.";
}

}  // namespace internal


// for testing only
std::ostream& operator<<(std::ostream& os, const StringPiece& piece)
{
	return os.write(piece.data(), static_cast<std::streamsize>(piece.size()));
}

}	// namespace annety
