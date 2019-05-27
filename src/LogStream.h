// Modify: Anny Wang
// Date: May 8 2019

#ifndef ANT_LOG_STREAM_H
#define ANT_LOG_STREAM_H

#include "BuildConfig.h"
#include "CompilerSpecific.h"
#include "ByteBuffer.h"

#include <iosfwd>
#include <cassert>
#include <string>
#include <limits>

namespace annety {
class LogStream {
public:
	static const int kMaxNumericSize = 32;
	
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
		"kMaxNumericSize is large enough");
public:
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


