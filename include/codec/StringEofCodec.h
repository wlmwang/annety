// By: wlmwang
// Date: Aug 20 2019

#ifndef ANT_CODEC_STRING_EOF_CODEC_H_
#define ANT_CODEC_STRING_EOF_CODEC_H_

#include "Macros.h"
#include "Logging.h"
#include "codec/Codec.h"
#include "strings/StringPiece.h"

namespace annety
{ 
// A codec that handle bytes of the following struct's streams:
// struct streams __attribute__ ((__packed__))
// {
// 	char payload[N];
// }
//
// Like as:
// hello\r\n
// world\r\n
//
// A Simple Fixed Tail Codec with Special Characters
class StringEofCodec : public Codec
{
public:
	StringEofCodec(EventLoop* loop, 
				   const std::string& string_eof = "\r\n", 
				   ssize_t max_payload = 64*1024*1024) 
		: Codec(loop)
		, inner_string_eof_(string_eof)
		, string_eof_(inner_string_eof_.data(), inner_string_eof_.size())
		, max_payload_(max_payload)
		, curr_offset_(0)
	{
		CHECK(string_eof.size() > 0);
		CHECK(max_payload > 0);
	}

	// Decode payload from |buff| to |payload|
	// NOTE: Has moved the read bytes from |buff| when decode success.
	// Returns:
	//   -1  decode error, going to close connection
	//    1  decode success, going to call message callback
	//    0  decode incomplete, continues to read more data
	// *Not thread safe*, but run in the own loop.
	virtual int decode(NetBuffer* buff, NetBuffer* payload) override
	{
		CHECK(!!buff && !!payload);

		int rt = 0;
		
		if (buff->readable_bytes() >= string_eof().size()) {
			StringPiece::size_type n = StringPiece::npos;
			if (string_eof().size() == 1) {
				// High performance (for single character).
				n = buff->to_string_piece().find(*string_eof().data(), curr_offset_);
			} else {
				n = buff->to_string_piece().find(string_eof(), curr_offset_);
			}
			curr_offset_ += buff->readable_bytes() - string_eof().size() - 1;

			if (n == StringPiece::npos && 
				max_payload() > 0 && buff->readable_bytes() > max_payload() + string_eof().size()) {
				LOG(ERROR) << "StringEofCodec::decode Invalid buffer bytes=" << buff->readable_bytes()
					<< ", max_payload=" << max_payload();
				rt = -1;
			} else if (n != StringPiece::npos) {
				// FIXME: Move bytes from |buff| to |payload|. (should be no-copy)
				payload->append(buff->begin_read(), n);
				buff->has_read(n + string_eof().size());
				curr_offset_ = 0;
				rt = 1;
			}
		}

		return rt;
	}
	
	// Encode stream bytes from |payload| to |buff|
	// NOTE: Do not remove the sent bytes from |payload| even if encode success.
	// Returns:
	//   -1  encode error, going to close connection
	//    1  encode success, going to send data to peer
	//    0  encode incomplete, continues to send more data
	// *Thread safe*, pure function.
	virtual int encode(NetBuffer* payload, NetBuffer* buff) override
	{
		CHECK(!!buff && !!payload);
		
		const ssize_t length = payload->readable_bytes();
		if (length == 0) {
			LOG(WARNING) << "StringEofCodec::encode Invalid length=0";
			return 0;
		} else if (max_payload() > 0 && length > max_payload()) {
			LOG(ERROR) << "StringEofCodec::encode Invalid length=" << length
				<< ", max_payload=" << max_payload();
			return -1;
		}

		// FIXME: Copy bytes from |payload| to |buff|. (should be no-copy)
		buff->append(payload->begin_read(), length);
		buff->append(string_eof().data(), string_eof().size());
		// Do not remove the sent bytes.
		// payload->has_read(length);

		return 1;
	}

private:
	StringPiece string_eof()
	{
		return string_eof_;
	}
	
	ssize_t max_payload()
	{
		return max_payload_;
	}
	
private:
	std::string inner_string_eof_;

	StringPiece string_eof_;
	ssize_t max_payload_;

	size_t curr_offset_;

	DISALLOW_COPY_AND_ASSIGN(StringEofCodec);
};

}	// namespace annety

#endif  // ANT_CODEC_STRING_EOF_CODEC_H_
