// Refactoring: Anny Wang
// Date: May 08 2019

#include "ByteBuffer.h"
#include "Logging.h"

// #include <iostream>	// std::cerr
#include <ostream>

namespace annety
{
void ByteBuffer::make_writable_bytes(size_t len)
{
	size_t real_writeable_bytes = writable_bytes();
	if (max_size_ != kUnLimitSize) {
		size_t real_size = std::min(static_cast<size_t>(max_size_), size());
		real_writeable_bytes = real_size - writer_index_;
	}

	if (real_writeable_bytes < len) {
		if (max_size_ != kUnLimitSize && max_size_ - writer_index_ < len) {
			// std::cerr << "not enough space to write"
			// 		<< "limit size:" << max_size_
			// 		<< "writable bytes:" << writable_bytes()
			// 		<< "append bytes" << len;
			return;
		}
		buffer_.resize(writer_index_+len);
	} else if (begin_read() != data()) {
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
