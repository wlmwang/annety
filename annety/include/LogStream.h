// By: wlmwang
// Date: May 08 2019

#ifndef ANT_LOG_STREAM_H
#define ANT_LOG_STREAM_H

#include "ByteBuffer.h"

#include <iosfwd>	// std::basic_ostream,std::char_traits
#include <limits>	// std::numeric_limits
#include <string>

namespace annety
{
// Example:
// // LogStream
// LogStream stream;
// stream << -12345.345 << "#" << string_printf("printf(%%3d) %3d", 555) << true;
// 
// LogStream stream1 = stream;	// copy
// stream.reset();	// stream1 unchange
//
// cout << "stream:" << stream << "|" << stream1 << endl;
// ...

class TimeStamp;
class TimeDelta;

// It is value sematics, which means that it can be copied or assigned.
class LogStream
{
	static const int  kMaxBufferSize = 4096;

	static const int kMaxNumericSize = 32;
	
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
		"kMaxNumericSize is large enough");
	static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
		"kMaxNumericSize is large enough");

	// For the function signature of std::endl
	//
	// See C++ STL define:
	// template<class CharT, class Traits>
	// std::basic_ostream<CharT, Traits>& endl(std::basic_ostream<CharT, Traits>& os);
	typedef std::basic_ostream<char, std::char_traits<char>> CharOStream;
	typedef CharOStream& (*StdEndLine) (CharOStream&);

public:
	LogStream(ssize_t max_size = kMaxBufferSize) : buffer_(max_size) {}
	
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

	LogStream& operator<<(const unsigned char* str)
	{
		return operator<<(reinterpret_cast<const char*>(str));
	}
	LogStream& operator<<(char v)
	{
		buffer_.append(&v, 1);
		return *this;
	}
	LogStream& operator<<(const char* str)
	{
		if (str) {
			buffer_.append(str, strlen(str));
		} else {
			buffer_.append("(*null*)", sizeof("(*null*)"));
		}
		return *this;
	}
	
	LogStream& operator<<(bool v)
	{
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}

	LogStream& operator<<(const std::string& v)
	{
		buffer_.append(v.c_str(), v.size());
		return *this;
	}
	LogStream& operator<<(const StringPiece& v)
	{
		buffer_.append(v.data(), v.size());
		return *this;
	}

	LogStream& operator<<(const ByteBuffer& v)
	{
		return operator<<(v.to_string_piece());
	}

	LogStream& operator<<(const StdEndLine&)
	{
		buffer_.append("\n", 1);
		return *this;
	}
	
	LogStream& operator<<(const TimeStamp& time);
	LogStream& operator<<(const TimeDelta& delta);

	// append to buffer
	void append(const char* data, size_t len)
	{
		buffer_.append(data, len);
	}

	void reset()
	{
		buffer_.reset();
	}
	
	const ByteBuffer& buffer() const
	{
		return buffer_;
	}

private:
	template<typename T>
	LogStream& format_number(T);

private:
	ByteBuffer buffer_;
};

std::ostream& operator<<(std::ostream& os, const LogStream& ls);

}	// namespace annety

#endif	// ANT_LOG_STREAM_H
