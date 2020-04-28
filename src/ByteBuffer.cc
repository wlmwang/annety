// By: wlmwang
// Date: May 08 2019

#include "ByteBuffer.h"
#include "Logging.h"

// #include <iostream>	// std::cerr
#include <ostream>

namespace annety
{
// buffer_ memory may be reallocated or migrated
void ByteBuffer::make_writable_bytes(size_t len)
{
	size_t real_writeable_bytes = writable_bytes();
	
	// fixed length ByteBuffer
	if (max_size_ != kUnLimitSize) {
		// real writeable bytes (has allocated memory of buffer_)
		size_t real_size = std::min(static_cast<size_t>(max_size_), size());
		real_writeable_bytes = real_size - writer_index_;
	}

	if (real_writeable_bytes < len) {
		// fixed length ByteBuffer
		if (max_size_ != kUnLimitSize && max_size_ - writer_index_ < len) {
			// std::cerr << "not enough space to write"
			// 		<< "limit size:" << max_size_
			// 		<< "writable bytes:" << writable_bytes()
			// 		<< "append bytes" << len;
			return;
		}

		// FIXME: should try to combine the read and writable bytes first
		buffer_.resize(writer_index_+len);
	} else if (begin_read() != data()) {
		migration_buffer_data();
	}
}

void ByteBuffer::migration_buffer_data()
{
	if (begin_read() != data()) {
		// move readable data to the front
		size_t readable = readable_bytes();
		std::copy(begin_read(), begin_write(), data());
		reader_index_ = 0;
		writer_index_ = reader_index_ + readable;
		assert(readable == readable_bytes());
	}
}

std::ostream& operator<<(std::ostream& os, const ByteBuffer& bb)
{
	return os << bb.to_string_piece();
}

}	// namespace annety
