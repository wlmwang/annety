// By: wlmwang
// Date: May 08 2019

#include "LogStream.h"
#include "TimeStamp.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <stddef.h>
#include <string.h>
#include <stdio.h>	// snprintf

namespace annety
{
namespace
{
const char kDigitMaps[] = "9876543210123456789";
const char kDigitHexMaps[] = "0123456789ABCDEF";

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value)
{
	T i = value;
	char* p = buf;

	const char* digits = kDigitMaps + 9;
	do {
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = digits[lsd];
	} while (i != 0);

	if (value < 0) {
		*p++ = '-';
	}

	*p = '\0';
	std::reverse(buf, p);
	return p - buf;
}

template<>
size_t convert<uintptr_t>(char buf[], uintptr_t value)
{
	uintptr_t i = value;
	char* p = buf;

	const char* digitsHex = kDigitHexMaps;
	do {
		int lsd = static_cast<int>(i % 16);
		i /= 16;
		*p++ = digitsHex[lsd];
	} while (i != 0);

	*p = '\0';
	std::reverse(buf, p);
	return p - buf;
}

}  // namespace anonymous

template<typename T>
LogStream& LogStream::format_number(T v)
{
	char buf[kMaxNumericSize+1];
	size_t len = convert(buf, v);
	buffer_.append(buf, len);
	return *this;
}

template <>
LogStream& LogStream::format_number<uintptr_t>(uintptr_t v)
{
	char buf[kMaxNumericSize+3] {'0', 'x'};
	size_t len = convert(buf+2, v);
	buffer_.append(buf, len+2);
	return *this;
}

// todo. replace this with Grisu3 by Florian Loitsch.
template <>
LogStream& LogStream::format_number<double>(double v)
{
	char buf[kMaxNumericSize+1];
	size_t len = ::snprintf(buf, kMaxNumericSize+1, "%.12g", v);
	buffer_.append(buf, len);
	return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
	return format_number(reinterpret_cast<uintptr_t>(p));
}

LogStream& LogStream::operator<<(float v)
{
	return *this << static_cast<double>(v);
}

LogStream& LogStream::operator<<(short v)
{
	return *this << static_cast<int>(v);
}

LogStream& LogStream::operator<<(unsigned short v)
{
	return *this << static_cast<unsigned int>(v);
}

LogStream& LogStream::operator<<(int v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(unsigned int v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(long v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(unsigned long v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(long long v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(unsigned long long v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(double v)
{
	return format_number(v);
}

LogStream& LogStream::operator<<(const TimeStamp& time)
{
	std::ostringstream oss;
	oss << time;
	return operator<<(oss.str());
}

LogStream& LogStream::operator<<(const TimeDelta& delta)
{
	std::ostringstream oss;
	oss << delta;
	return operator<<(oss.str());
}

// for testing only
std::ostream& operator<<(std::ostream& os, const LogStream& ls)
{
	return os << ls.buffer();
}

}	// namespace annety
