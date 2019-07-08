// Refactoring: Anny Wang
// Date: Jun 22 2019

#ifndef ANT_NET_BUFFER_H
#define ANT_NET_BUFFER_H

#include "ByteOrder.h"

#include <iosfwd>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

namespace annety
{
class NetBuffer : public ByteBuffer
{
public:
	// inheritance of construct
	using ByteBuffer::ByteBuffer;
	explicit NetBuffer() : ByteBuffer() {}

	// copy-ctor, move-ctor, dtor and assignment
	NetBuffer(const NetBuffer&) = default;
	NetBuffer(NetBuffer&&) = default;
	NetBuffer& operator=(const NetBuffer&) = default;
	NetBuffer& operator=(NetBuffer&&) = default;
	~NetBuffer() = default;
  
	const char* peek() const { return begin_read();}

	// Append int* to buffer ----------------------------------
	void append_int64(int64_t x)
	{
		int64_t be64 = host_to_net64(x);
		append(&be64, sizeof be64);
	}
	void append_int32(int32_t x)
	{
		int32_t be32 = host_to_net32(x);
		append(&be32, sizeof be32);
	}
	void append_int16(int16_t x)
	{
		int16_t be16 = host_to_net16(x);
		append(&be16, sizeof be16);
	}
	void append_int8(int8_t x)
	{
		append(&x, sizeof x);
	}

	// Read int* from buffer ----------------------------------
	int64_t read_int64()
	{
		int64_t result = peek_int64();
		has_read_int64();
		return result;
	}
	int32_t read_int32()
	{
		int32_t result = peek_int32();
		has_read_int32();
		return result;
	}
	int16_t read_int16()
	{
		int16_t result = peek_int16();
		has_read_int16();
		return result;
	}
	int8_t read_int8()
	{
		int8_t result = peek_int8();
		has_read_int8();
		return result;
	}

	// Require: readable_bytes() >= sizeof(int64_t)
	int64_t peek_int64() const
	{
		assert(readable_bytes() >= sizeof(int64_t));
		int64_t be64 = 0;
		::memcpy(&be64, begin_read(), sizeof be64);
		return net_to_host64(be64);
	}

	// Require: readable_bytes() >= sizeof(int32_t)
	int32_t peek_int32() const
	{
		assert(readable_bytes() >= sizeof(int32_t));
		int32_t be32 = 0;
		::memcpy(&be32, begin_read(), sizeof be32);
		return net_to_host32(be32);
	}
	
	// Require: readable_bytes() >= sizeof(int16_t)
	int16_t peek_int16() const
	{
		assert(readable_bytes() >= sizeof(int16_t));
		int16_t be16 = 0;
		::memcpy(&be16, begin_read(), sizeof be16);
		return net_to_host16(be16);
	}

	// Require: readable_bytes() >= sizeof(int8_t)
	int8_t peek_int8() const
	{
		assert(readable_bytes() >= sizeof(int8_t));
		int8_t be8 = 0;
		::memcpy(&be8, begin_read(), sizeof be8);
		return be8;
	}

	void has_read_int64() { has_read(sizeof(int64_t));}
	void has_read_int32() { has_read(sizeof(int32_t));}
	void has_read_int16() { has_read(sizeof(int16_t));}
	void has_read_int8() { has_read(sizeof(int8_t));}

	ssize_t read_fd(int fd, int* err = nullptr);
};

}	// namespace annety

#endif	// ANT_NET_BUFFER_H
