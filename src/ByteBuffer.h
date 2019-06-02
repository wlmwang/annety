// Modify: Anny Wang
// Date: May 8 2019

#ifndef ANT_BYTE_BUFFER_H
#define ANT_BYTE_BUFFER_H

#include "BuildConfig.h"
#include "CompilerSpecific.h"
#include "StringPiece.h"

#include <iosfwd>
#include <iostream>

#include <algorithm>
#include <vector>
#include <string>
#include <string.h>
#include <assert.h>

namespace annety {
// @code
// +-------------------+------------------+------------------+
// | prependable bytes |  readable bytes  |  writable bytes  |
// |                   |     (CONTENT)    |                  |
// +-------------------+------------------+------------------+
// |                   |                  |                  |
// 0      <=      readerIndex   <=   writerIndex    <=     size
// @endcode
template<ssize_t LIMIT_SIZE = -1>
class ByteBuffer {
public:
  static const size_t kInitialSize = 1024;

  static_assert(LIMIT_SIZE >= -1 && LIMIT_SIZE != 0, 
                "illegal LIMIT SIZE buffer");
  
  explicit ByteBuffer(size_t initialSize = kInitialSize)
    : buffer_(LIMIT_SIZE>0? LIMIT_SIZE: initialSize),
      readerIndex_(0),
      writerIndex_(0) {}

  void swap(ByteBuffer& rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  size_t readable_bytes() const {
    assert(writerIndex_ >= readerIndex_);
    return writerIndex_ - readerIndex_;
  }

  size_t writable_bytes() const {
    if (LIMIT_SIZE != -1) {
      assert(LIMIT_SIZE >= writerIndex_);
      // todo
      assert(size() >= writerIndex_);
      return LIMIT_SIZE - writerIndex_;
    }
    assert(size() >= writerIndex_);
    return size() - writerIndex_; 
  }
  
  char *begin_read() {
    assert(readerIndex_ <= writerIndex_);
    return data() + readerIndex_;
  }
  const char *begin_read() const {
    assert(readerIndex_ <= writerIndex_);
    return data() + readerIndex_;
  }

  char* begin_write() {
    assert(writerIndex_ <= size());
    return data() + writerIndex_;
  }
  const char* begin_write() const {
    assert(writerIndex_ <= size());
    return data() + writerIndex_;
  }
  
  void has_written(size_t len) {
    assert(writable_bytes() >= len);
    writerIndex_ += len;
  }

  void has_read(size_t len) {
    if (len < readable_bytes()) {
      readerIndex_ += len;
    } else {
      reset();
    }
  }

  void reset() {
    readerIndex_ = 0;
    writerIndex_ = 0;
  }

  // zero-copy.
  // But use this carefully(about ownership). 
  // And sometimes need call has_read() youself
  StringPiece to_string_piece() const {
    return StringPiece(begin_read(), readable_bytes());
  }

  std::string to_string() const {
    return std::string(begin_read(), readable_bytes());
  }

  std::string taken_as_string(ssize_t len = -1) {
    if (len <= -1) {
      len = readable_bytes();
    }
    assert(len <= readable_bytes());
    std::string result(begin_read(), len);
    has_read(static_cast<size_t>(len));
    return result;
  }

  bool append(const StringPiece& str) {
    return append(str.data(), str.size());
  }
  bool append(const void* data, size_t len) {
    return append(static_cast<const char*>(data), len);
  }
  bool append(const char* data, size_t len) {
    ensure_writable_bytes(len);
    if (writable_bytes() >= len) {
      std::copy(data, data + len, begin_write());
      has_written(len);
      return true;
    }
    return false;
  }

  void shrink(size_t reserve) {
    buffer_.shrink_to_fit();
    if (reserve) {
      buffer_.reserve(reserve);
    }
  }

  void ensure_writable_bytes(size_t len) {
    make_writable_bytes(len);
  }

private:
  char* data() {
    return buffer_.data();
  }
  const char* data() const {
    return buffer_.data();
  }

  size_t capacity() const {
    return buffer_.capacity();
  }
  size_t size() const {
    return buffer_.size();
  }
  
  void make_writable_bytes(size_t len) {
    size_t real_writeable_bytes = writable_bytes();
    if (LIMIT_SIZE != -1) {
      size_t real_size = std::min(static_cast<size_t>(LIMIT_SIZE), size());
      real_writeable_bytes = real_size - writerIndex_;
    }

    if (real_writeable_bytes < len) {
      if (LIMIT_SIZE != -1 && LIMIT_SIZE - writerIndex_ < len) {
        // std::stderr << "Not enough space to write"
        //             << "limit size:" << LIMIT_SIZE
        //             << "writable bytes:" << writable_bytes()
        //             << "append bytes" << len;
        return;
      }
      buffer_.resize(writerIndex_+len);
    } else if (begin_read() != data()) {
      // move readable data to the front, make space inside buffer
      size_t readable = readable_bytes();
      std::copy(begin_read(), begin_write(), data());
      readerIndex_ = 0;
      writerIndex_ = readerIndex_ + readable;
      assert(readable == readable_bytes());
    }
  }

private:
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};

// template instance
typedef ByteBuffer<-1> UnboundedBuffer;
typedef ByteBuffer<4 * 1024> LogBuffer;
typedef ByteBuffer<4 * 1024 * 1000> LLogBuffer;

// For testing only
template<ssize_t LIMIT_SIZE>
std::ostream& operator<<(std::ostream& os, const ByteBuffer<LIMIT_SIZE>& bb) {
  return os << bb.to_string_piece();
}

}  // namespace annety

#endif  // ANT_BYTE_BUFFER_H
