// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 8 2019

#ifndef _ANT_STRING_PIECE_H_
#define _ANT_STRING_PIECE_H_

#include "BuildConfig.h"
#include "CompilerSpecific.h"

#include <iterator>
#include <iosfwd>
#include <string>
#include <cstring>
#include <assert.h>

namespace annety {
// StringPiece ------------------------------------------------------------

// Defines the types, methods, operators, and data members common to both
// StringPiece. Do not refer to this class directly, but rather to StringPiece.
class StringPiece {
public:
	typedef size_t size_type;
  	typedef typename std::string::value_type value_type;	// usually char
  	typedef const value_type* pointer;
  	typedef const value_type* const_iterator;
  	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	
	static const size_type npos = -1;

public:
  	constexpr StringPiece() : ptr_(nullptr), length_(0) {}

  	constexpr StringPiece(const value_type* str)
  		: ptr_(str), length_(!str ? 0 : strlen(reinterpret_cast<const char*>(str))) {}

    StringPiece(const std::string& str)
    	: ptr_(str.data()), length_(str.size()) {}
  	
  	constexpr StringPiece(const value_type* offset, size_type len)
  		: ptr_(offset), length_(len) {}
  	
  	StringPiece(const const_iterator& begin, const const_iterator& end) {
    	length_ = static_cast<size_t>(std::distance(begin, end));
    	ptr_ = length_ > 0 ? &*begin : nullptr;
  	}

  	constexpr const value_type* data() const { return ptr_; }
  	constexpr size_type size() const { return length_; }
  	constexpr size_type length() const { return length_; }
  	bool empty() const { return length_ == 0; }

	const_iterator begin() const { return ptr_; }
	const_iterator end() const { return ptr_ + length_; }
	const_reverse_iterator rbegin() const {
		return const_reverse_iterator(ptr_ + length_);
	}
	const_reverse_iterator rend() const {
		return const_reverse_iterator(ptr_);
	}

	size_type max_size() const { return length_; }
	size_type capacity() const { return length_; }

  	void clear() {
  		ptr_ = NULL;
  		length_ = 0;
	}

	void set(const value_type* str) {
		ptr_ = str;
		length_ = str ? strlen(reinterpret_cast<const char*>(str)): 0;
	}
	void set(const value_type* data, size_type len) {
		ptr_ = data;
		length_ = len;
	}
	void set(const void* str, size_type len) {
		ptr_ = reinterpret_cast<const value_type*>(str);
		length_ = len;
	}

	value_type operator[](size_type i) const {
		assert(i < length_);
		return ptr_[i];
	}

	value_type front() const {
		assert(0UL != length_);
		return ptr_[0];
	}
	value_type back() const {
		assert(0UL != length_);
		return ptr_[length_ - 1];
	}

	void remove_prefix(size_type n) {
		assert(n <= length_);
		ptr_ += n;
		length_ -= n;
	}
	void remove_suffix(size_type n) {
		assert(n <= length_);
		length_ -= n;
	}

	constexpr bool starts_with(StringPiece x) const noexcept {
		return ((length_ >= x.length_) && 
				(memcmp(ptr_, x.ptr_, x.length_) == 0));
	}

	constexpr bool ends_with(StringPiece x) const noexcept {
		return ((length_ >= x.length_) && 
				(memcmp(ptr_ + (length_ - x.length_), x.ptr_, x.length_) == 0));
	}

	// substr.
	StringPiece substr(size_type pos, size_type n = StringPiece::npos) const {
		assert(pos < length_);
		return StringPiece(ptr_ + pos, ptr_ + (n <= length_? n : length_));
	}

	std::string as_string() const {
		return empty() ? std::string() : std::string(data(), size());
	}

	void copy_to_string(std::string* target) const {
		target->assign(ptr_, length_);
	}
	void append_to_string(std::string* target) const {
		target->append(ptr_, length_);
	}

  	// find: Search for a character or substring at a given offset.
	size_type find(const StringPiece& s, size_type pos = 0) const {
		return as_string().find(s.ptr_, pos);
	}
	size_type find(value_type c, size_type pos = 0) const {
		return as_string().find(c, pos);
	}

	// rfind: Reverse find.
	size_type rfind(const StringPiece& s, size_type pos = StringPiece::npos) const {
		return as_string().rfind(s.ptr_, pos);
	}
	size_type rfind(value_type c, size_type pos = StringPiece::npos) const {
		return as_string().rfind(c, pos);
	}

  	// find_first_of: Find the first occurence of one of a set of characters.
	size_type find_first_of(const StringPiece& s, size_type pos = 0) const {
		return as_string().find_first_of(s.ptr_, pos);
	}
	size_type find_first_of(value_type c, size_type pos = 0) const {
		return find(c, pos);
	}

	// find_first_not_of: Find the first occurence not of a set of characters.
	size_type find_first_not_of(const StringPiece& s, size_type pos = 0) const {
		return as_string().find_first_not_of(s.ptr_, pos);
	}
	size_type find_first_not_of(value_type c, size_type pos = 0) const {
		return as_string().find_first_not_of(c, pos);
	}

	// find_last_of: Find the last occurence of one of a set of characters.
	size_type find_last_of(const StringPiece& s, size_type pos = StringPiece::npos) const {
		return as_string().find_last_of(s.ptr_, pos);
	}
	size_type find_last_of(value_type c, size_type pos = StringPiece::npos) const {
		return rfind(c, pos);
	}

	// find_last_not_of: Find the last occurence not of a set of characters.
	size_type find_last_not_of(const StringPiece& s, size_type pos = StringPiece::npos) const {
		return as_string().find_last_not_of(s.ptr_, pos);
	}
	size_type find_last_not_of(value_type c, size_type pos = StringPiece::npos) const {
		return as_string().find_last_not_of(c, pos);
	}

	int compare(const StringPiece& x) const noexcept {
		int r = memcmp(ptr_, x.ptr_, length_ < x.length_? length_: x.length_);
		if (r == 0) {
			if (length_ < x.length_) {
				r = -1;
			} else if (length_ > x.length_) {
				r = 1;
			}
		}
		return r;
	}

	bool operator==(const StringPiece& x) const {
		return ((length_ == x.length_) &&
			(memcmp(ptr_, x.ptr_, length_) == 0));
	}
	bool operator!=(const StringPiece& x) const {
		return !(*this == x);
	}

#define STRINGPIECE_BINARY_PREDICATE(op, auxcmp)	\
	bool operator op(const StringPiece& x) const {	\
		int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);	\
		return ((r auxcmp 0) || ((r == 0) && (length_ op x.length_)));	\
	}
	STRINGPIECE_BINARY_PREDICATE(<,  <)
	STRINGPIECE_BINARY_PREDICATE(<=, <)
	STRINGPIECE_BINARY_PREDICATE(>=, >)
	STRINGPIECE_BINARY_PREDICATE(>,  >)
#undef STRINGPIECE_BINARY_PREDICATE

private:
	pointer ptr_;
	size_type length_;
};

// StingPiece ostream --------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const StringPiece& piece);

// Hashing ---------------------------------------------------------------------

// We provide appropriate hash functions, so StringPiece can
// be used as keys in hash sets and maps.
#define HASH_STRING_PIECE(StringPieceType, string_piece)         \
  std::size_t result = 0;                                        \
  for (StringPieceType::const_iterator i = string_piece.begin(); \
       i != string_piece.end(); ++i)                             \
    result = (result * 131) + *i;                                \
  return result;

struct StringPieceHash {
  std::size_t operator()(const StringPiece& sp) const {
    HASH_STRING_PIECE(StringPiece, sp);
  }
};

}  // namespace annety

#endif

