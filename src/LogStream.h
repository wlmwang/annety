// Refactoring: Anny Wang
// Date: May 08 2019

#ifndef ANT_LOG_STREAM_H
#define ANT_LOG_STREAM_H

#include "ByteBuffer.h"

#include <iosfwd>
#include <string>
#include <limits>

namespace annety {
class LogStream {
	static const int kMaxNumericSize = 32;
	
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
		"kMaxNumericSize is large enough");

	// for the function signature of std::endl
	// STL:
	// template<class CharT, class Traits>
	// std::basic_ostream<CharT, Traits>& endl(std::basic_ostream<CharT, Traits>& os);
	typedef std::basic_ostream<char, std::char_traits<char>> CharOStream;
	typedef CharOStream& (*StdEndLine) (CharOStream&);

public:
	LogStream() {}
	
	// copy-ctor, move-ctor, dtor and assignment
	LogStream(const LogStream&) = default;
	LogStream(LogStream&&) = default;
	LogStream& operator=(const LogStream&) = default;
	LogStream& operator=(LogStream&&) = default;
	~LogStream() = default;

	// Ostream operators
	LogStream& operator<<(const void*);
	
	LogStream& operator<<(float);
	LogStream& operator<<(double);
	
	LogStream& operator<<(short);
	LogStream& operator<<(unsigned short);
	LogStream& operator<<(int);
	LogStream& operator<<(unsigned int);
	LogStream& operator<<(long);
	LogStream& operator<<(unsigned long);
	LogStream& operator<<(long long);
	LogStream& operator<<(unsigned long long);

	LogStream& operator<<(const unsigned char* str) {
		return operator<<(reinterpret_cast<const char*>(str));
	}
	LogStream& operator<<(char v) {
		buffer_.append(&v, 1);
		return *this;
	}
	LogStream& operator<<(const char* str) {
		if (str) {
			buffer_.append(str, strlen(str));
		} else {
			buffer_.append("(*null*)", sizeof("(*null*)"));
		}
		return *this;
	}
	
	LogStream& operator<<(bool v) {
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}

	LogStream& operator<<(const std::string& v) {
		buffer_.append(v.c_str(), v.size());
		return *this;
	}
	LogStream& operator<<(const StringPiece& v) {
		buffer_.append(v.data(), v.size());
		return *this;
	}

	LogStream& operator<<(const LogBuffer& v) {
		return operator<<(v.to_string_piece());
	}

	LogStream& operator<<(const StdEndLine&) {
		buffer_.append("\n", 1);
		return *this;
	}

	// append to buffer
	void append(const char* data, size_t len) {
		buffer_.append(data, len);
	}

	void reset() {
		buffer_.reset();
	}
	
	const LogBuffer& buffer() const {
		return buffer_;
	}

private:
	template<typename T>
	LogStream& format_number(T);

private:
	LogBuffer buffer_;
};

// For testing only
std::ostream& operator<<(std::ostream& os, const LogStream& ls);

}	// namespace annety

#endif	// ANT_LOG_STREAM_H
