// By: wlmwang
// Date: Aug 15 2019

#ifndef ANT_CODEC_LENGTH_HEADER_CODEC_H_
#define ANT_CODEC_LENGTH_HEADER_CODEC_H_

#include "Macros.h"
#include "codec/Codec.h"

namespace annety
{ 
// A codec that handle bytes of the following struct's streams:
// struct streams
// {
// 	uint32_t length;
// 	char body[0];
// }

// A Simple Fixed Head Codec with Length
class LengthHeaderCodec : public Codec
{
public:
	enum LENGTH_TYPE
	{
		kLengthType8	= 1,
		kLengthType16	= 2,
		kLengthType32	= 4,
		kLengthType64	= 8,
	};

	explicit LengthHeaderCodec(EventLoop* loop, 
							   LENGTH_TYPE length_type, 
							   ssize_t max_payload = 0) 
		: Codec(loop)
		, length_type_(length_type)
		, max_payload_(max_payload)
	{
		DCHECK(length_type == kLengthType8 || 
			length_type == kLengthType16 || 
			length_type == kLengthType32 || 
			length_type == kLengthType64);
	}

	LENGTH_TYPE length_type()
	{
		return length_type_;
	}
	
	ssize_t max_payload()
	{
		return max_payload_;
	}

	// Decode payload from |buff| to |payload|
	// NOTE: You must be remove the read bytes from |buff|
	// Returns:
	//   -1  decode error, going to close connection
	//    1  decode success, going to call message callback when decode success
	//    0  decode incomplete, continues to read more data
	virtual int decode(NetBuffer* buff, NetBuffer* payload) override
	{
		auto get_payload_length = [this] (const NetBuffer* buff) {
			int64_t length = -1;
			switch (length_type()) {
			case kLengthType8:
				length = buff->peek_int8();
				break;

			case kLengthType16:
				length = buff->peek_int16();
				break;

			case kLengthType32:
				length = buff->peek_int32();
				break;

			case kLengthType64:
				length = buff->peek_int64();
				break;
			}

			return length;
		};

		int rt = 0;
		if (buff->readable_bytes() >= length_type()) {
			const ssize_t length = get_payload_length(buff);
			if (length < 0 || (max_payload() > 0 && length > max_payload())) {
				LOG(ERROR) << "LengthHeaderCodec::decode Invalid length=" << length
					<< ", max_payload=" << max_payload();
				rt = -1;
			} else if (buff->readable_bytes() >= static_cast<size_t>(length + length_type())) {
				// FIXME: move bytes from |buff| to |payload|.
				payload->append(buff->begin_read() + length_type(), length);
				buff->has_read(length_type() + length);
				rt = 1;
			}
		}
		return rt;
	}
	
	// Encode stream bytes from |payload| to |buff|
	// NOTE: You must be remove the sent bytes from |payload| when encode success
	// Returns:
	//   -1  encode error, going to close connection
	//    1  encode success, going to send data to peer
	//    0  encode incomplete, continues to send more data
	virtual int encode(NetBuffer* payload, NetBuffer* buff) override
	{
		auto set_buff_length = [this] (const NetBuffer* payload, NetBuffer* buff) {
			switch (length_type()) {
			case kLengthType8:
				buff->append_int8(payload->readable_bytes());
				break;

			case kLengthType16:
				buff->append_int16(payload->readable_bytes());
				break;

			case kLengthType32:
				buff->append_int32(payload->readable_bytes());
				break;

			case kLengthType64:
				buff->append_int64(payload->readable_bytes());
				break;
			}
		};

		const ssize_t length = payload->readable_bytes();
		if (length == 0) {
			LOG(WARNING) << "LengthHeaderCodec::encode Invalid length=0";
			return 0;
		} else if (length < 0 || (max_payload() > 0 && length > max_payload())) {
			LOG(ERROR) << "LengthHeaderCodec::encode Invalid length=" << length
				<< ", max_payload=" << max_payload();
			return -1;
		}

		// FIXME: move bytes from |payload| to |buff|
		set_buff_length(payload, buff);
		buff->append(payload->begin_read(), length);
		payload->has_read_all();
		return 1;
	}

private:
	LENGTH_TYPE length_type_;
	ssize_t max_payload_;
	
	DISALLOW_COPY_AND_ASSIGN(LengthHeaderCodec);
};

}	// namespace annety

#endif  // ANT_CODEC_LENGTH_HEADER_CODEC_H_
