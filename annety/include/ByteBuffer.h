// By: wlmwang
// Date: May 08 2019

#ifndef ANT_BYTE_BUFFER_H
#define ANT_BYTE_BUFFER_H

#include "strings/StringPiece.h"

#include <vector>
#include <string>
#include <iosfwd>		// std::ostream
#include <algorithm>	// std::swap
#include <stddef.h>		// ssize_t,size_t
#include <assert.h>		// assert

namespace annety
{
// Example:
// // ByteBuffer
// ByteBuffer buff{10};
// buff.append("123456");
// buff.append("ab");
// buff.append("cd");
//
// ByteBuffer buff1 = buff;
//
// cout << "take before:" << buff << endl;
// cout << "take all:" << buff.taken_as_string() << endl;	// taken
// cout << "take after:" << buff << "|" << buff1 << endl;	// buff1 unchange
// 
// buff.append("01");
// buff.append("23");
//
// cout << "append success:" << buff.append("4567890") << endl;
// cout << "append success:" << buff.append("a") << endl;
// cout << "append rt:" << buff << endl;
//
// ByteBuffer buff2 = std::move(buff);	// buff is empty
// cout << "move ByteBuffer:" << buff2 << "|" << buff << endl;
// ...

// FIXME: ByteBuffer cannot use CHECK macros, because CHECK Low-level 
// implementation is depend ByteBuffer. so we use assert() macros here.
//
// FIXME: Fixed-length ByteBuffer will be allocated memory immediately 
// after initialization.
//
// It is value sematics, which means that it can be copied or assigned.
// *Not thread safe*
//
// @coding
// +-------------------+------------------+------------------+
// |  has read bytes   |  readable bytes  |  writable bytes  |
// |                   |     (CONTENT)    |                  |
// +-------------------+------------------+------------------+
// |                   |                  |                  |
// 0      <=      readerIndex   <=   writerIndex    <=     size
// @coding
class ByteBuffer
{
public:
	static const ssize_t kUnLimitSize = -1;
	static const size_t  kInitialSize = 1024;

	explicit ByteBuffer(ssize_t max_size = kUnLimitSize, size_t init_size = kInitialSize)
				: max_size_(max_size)
				, buffer_(max_size>0? max_size: init_size) {}

	// copy, dtor and assignment
	ByteBuffer(const ByteBuffer&) = default;
	ByteBuffer& operator=(const ByteBuffer&) = default;
	~ByteBuffer() = default;
	
	// move method
	ByteBuffer(ByteBuffer&& rhs) 
		: max_size_(rhs.max_size_)
		, reader_index_(rhs.reader_index_)
		, writer_index_(rhs.writer_index_)
		, buffer_(std::move(rhs.buffer_))
	{
		rhs.reset();
	}
	ByteBuffer& operator=(ByteBuffer&& rhs)
	{
		swap(rhs);
		rhs.reset();
		return *this;
	}

	void swap(ByteBuffer& rhs)
	{
		std::swap(max_size_, rhs.max_size_);
		std::swap(reader_index_, rhs.reader_index_);
		std::swap(writer_index_, rhs.writer_index_);
		buffer_.swap(rhs.buffer_);
	}
	
	void reset()
	{
		reader_index_ = 0;
		writer_index_ = 0;
	}

	size_t readable_bytes() const
	{
		assert(writer_index_ >= reader_index_);
		return writer_index_ - reader_index_;
	}

	// Use this carefully (must allocate max_size_ memory when ByteBuffer initialize).
	size_t writable_bytes() const
	{
		assert(size() >= writer_index_);
		size_t vbytes = size();

		// fixed length ByteBuffer
		if (max_size_ != kUnLimitSize) {
			vbytes = max_size_;
			assert(vbytes >= writer_index_);
		}
		return vbytes - writer_index_; 
	}

	char *begin_read()
	{
		assert(reader_index_ <= writer_index_);
		return data() + reader_index_;
	}
	const char *begin_read() const
	{
		assert(reader_index_ <= writer_index_);
		return data() + reader_index_;
	}

	char* begin_write()
	{
		assert(writer_index_ <= size());
		return data() + writer_index_;
	}
	const char* begin_write() const
	{
		assert(writer_index_ <= size());
		return data() + writer_index_;
	}

	void has_written(size_t len)
	{
		assert(writable_bytes() >= len);
		writer_index_ += len;
	}

	void has_read(size_t len)
	{
		if (len < readable_bytes()) {
			reader_index_ += len;
		} else {
			reset();
		}
	}
	
	void has_read_all()
	{
		reset();
	}

	std::string to_string() const
	{
		return std::string(begin_read(), readable_bytes());
	}
	// Use this carefully (about ownership).
	StringPiece to_string_piece() const
	{
		return StringPiece(begin_read(), readable_bytes());
	}

	// -1 means taken all byte data
	std::string taken_as_string(ssize_t len = -1)
	{
		if (len <= -1) {
			len = readable_bytes();
		}
		assert(static_cast<size_t>(len) <= readable_bytes());
		
		std::string result(begin_read(), len);
		has_read(static_cast<size_t>(len));
		return result;
	}

	// buffer_ memory may be reallocated or migrated
	bool append(const StringPiece& str)
	{
		return append(str.data(), str.size());
	}
	bool append(const void* data, size_t len)
	{
		return append(static_cast<const char*>(data), len);
	}
	bool append(const char* data, size_t len)
	{
		ensure_writable_bytes(len);
		if (writable_bytes() >= len) {
			std::copy(data, data + len, begin_write());
			has_written(len);
			return true;
		}
		return false;
	}
	
	// buffer_ memory may be reallocated or migrated
	void ensure_writable_bytes(size_t len)
	{
		make_writable_bytes(len);
	}

	void shrink()
	{
		migration_buffer_data();
		buffer_.resize(readable_bytes());
		buffer_.shrink_to_fit();
	}

protected:
	// The following interfaces are not recommended. Please use them carefully
	size_t capacity() const { return buffer_.capacity();}
	size_t size() const { return buffer_.size();}

	char* data() { return buffer_.data();}
	const char* data() const { return buffer_.data();}

	void make_writable_bytes(size_t len);
	
	void migration_buffer_data();

private:
	ssize_t max_size_{kUnLimitSize};
	size_t reader_index_{0};
	size_t writer_index_{0};
	std::vector<char> buffer_;
};

std::ostream& operator<<(std::ostream& os, const ByteBuffer& bb);

}	// namespace annety

#endif  // ANT_BYTE_BUFFER_H
