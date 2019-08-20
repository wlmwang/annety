// Refactoring: Anny Wang
// Date: Aug 20 2019

#ifndef ANT_CODEC_STRING_EOF_CODEC_H_
#define ANT_CODEC_STRING_EOF_CODEC_H_

#include "Macros.h"
#include "codec/Codec.h"
#include "strings/StringPiece.h"

namespace annety
{ 
// A codec that handle bytes of the following struct's streams:
// hello\r\n
// world\r\n

// A Simple Fixed Tail Codec with Special Characters
class StringEofCodec : public Codec
{
public:
	static const ssize_t kLimitSize = 1*1024*1024*1024;

	explicit StringEofCodec(const std::string& string_eof, ssize_t max_payload = 512*1024) 
		: inner_string_eof_(string_eof)
		, string_eof_(inner_string_eof_.data(), inner_string_eof_.size())
		, max_payload_(max_payload)
		, curr_offset_(0)
	{
		CHECK(string_eof.size() > 0);
		CHECK(max_payload <= kLimitSize);
	}

	StringPiece string_eof()
	{
		return string_eof_;
	}
	
	ssize_t max_payload()
	{
		return max_payload_;
	}

	// Decode payload from |buff| to |payload|
	virtual int decode(NetBuffer* buff, NetBuffer* payload) override
	{
		int rt = 0;
		if (buff->readable_bytes() >= string_eof().size()) {
			// FIXME: Save each decode offset
			const StringPiece::size_type n = buff->to_string_piece().find(string_eof(), 0);
			if (n == StringPiece::npos && 
				max_payload() > 0 && buff->readable_bytes() > max_payload() + string_eof().size()) {
				LOG(WARNING) << "StringEofCodec::decode Invalid buffer bytes=" << buff->readable_bytes()
					<< ", max_payload=" << max_payload();
				rt = -1;
			} else if (n != StringPiece::npos) {
				payload->append(buff->begin_read(), n);
				buff->has_read(n + string_eof().size());
				rt = 1;
			}
		}
		return rt;
	}
	
	// Encode stream bytes from |payload| to |buff|
	virtual int encode(NetBuffer* payload, NetBuffer* buff) override
	{
		const ssize_t length = payload->readable_bytes();
		if (length == 0) {
			LOG(WARNING) << "StringEofCodec::encode Invalid length=0";
			return 0;
		} else if (max_payload() > 0 && length > max_payload()) {
			LOG(WARNING) << "StringEofCodec::encode Invalid length=" << length
				<< ", max_payload=" << max_payload();
			return -1;
		}

		// FIXME: move bytes from |payload| to |buff|
		buff->append(payload->begin_read(), length);
		buff->append(string_eof().data(), string_eof().size());
		payload->has_read_all();
		return 1;
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
