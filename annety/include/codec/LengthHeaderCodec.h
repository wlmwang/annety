// By: wlmwang
// Date: Aug 15 2019

#ifndef ANT_CODEC_LENGTH_HEADER_CODEC_H_
#define ANT_CODEC_LENGTH_HEADER_CODEC_H_

#include "build/CompilerSpecific.h"
#include "Macros.h"
#include "Logging.h"
#include "ByteOrder.h"
#include "Crc32c.h"
#include "codec/Codec.h"

namespace annety
{
namespace
{
inline uint32_t peek_uint32(const char* buff)
{
	uint32_t be32 = 0;
	::memcpy(&be32, buff, sizeof be32);
	return net_to_host32(be32);
}
}	// namespace anonymous

// A codec that handle bytes of the following struct's streams:
// struct streams __attribute__ ((__packed__))
// {
// 	uint32_t length;
// 	char payload[length];
// }
//
// Like as:
// | 11 | hello-world |
//
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
							   bool enable_checksum = true,
							   ssize_t max_payload = 64*1024*1024) 
		: Codec(loop)
		, length_type_(length_type)
		, max_payload_(max_payload)
	{
		DCHECK(length_type == kLengthType8 || 
			length_type == kLengthType16 || 
			length_type == kLengthType32 || 
			length_type == kLengthType64);

		checksum_length_ = !enable_checksum? 0: sizeof(uint32_t);
	}

	// Decode payload from |buff| to |payload|
	// NOTE: You must be remove the read bytes from |buff|
	// Returns:
	//   -1  decode error, going to close connection
	//    1  decode success, going to call message callback when decode success
	//    0  decode incomplete, continues to read more data
	virtual int decode(NetBuffer* buff, NetBuffer* payload) override
	{
		DCHECK(!!buff && !!payload);

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
			if (length < min_payload() || (max_payload() > 0 && length > max_payload())) {
				LOG(ERROR) << "LengthHeaderCodec::decode Invalid length=" << length
					<< ", max_payload=" << max_payload();
				rt = -1;
			} else if (buff->readable_bytes() >= static_cast<size_t>(length + length_type())) {
				uint32_t expectsum = 0, checksum = 0;

				// turn on/off crc32 checksum
				if (LIKELY(checksum_length() > 0)) {
					checksum = peek_uint32(buff->begin_read() + length_type() + length - checksum_length());

					using crc32_func_t = uint32_t(*)(const char*, size_t);
					crc32_func_t crc32_func = nullptr;
					if (LIKELY(length - checksum_length() > 60)) {
						crc32_func = Crc32c::crc32_long;
					} else {
						crc32_func = Crc32c::crc32_short;
					}
					expectsum = crc32_func(buff->begin_read() + length_type(), length - checksum_length());
				}

				if (LIKELY(expectsum == checksum)) {
					// FIXME: move bytes from |buff| to |payload|. (not copy)
					payload->append(buff->begin_read() + length_type(), length - checksum_length());
					buff->has_read(length_type() + length);
					rt = 1;
				} else {
					LOG(ERROR) << "LengthHeaderCodec::decode Invalid checksum=" << checksum
						<< ", expectsum=" << expectsum;
					rt = -1;
				}
			}
		}

		return rt;
	}
	
	// Encode stream bytes from |payload| to |buff|
	// NOTE: You must be remove the sent bytes from |payload|
	// Returns:
	//   -1  encode error, going to close connection
	//    1  encode success, going to send data to peer
	//    0  encode incomplete, continues to send more data
	virtual int encode(NetBuffer* payload, NetBuffer* buff) override
	{
		DCHECK(!!buff && !!payload);
		
		auto set_buff_length = [this] (ssize_t length, NetBuffer* buff) {
			switch (length_type()) {
			case kLengthType8:
				buff->append_int8(length);
				break;

			case kLengthType16:
				buff->append_int16(length);
				break;

			case kLengthType32:
				buff->append_int32(length);
				break;

			case kLengthType64:
				buff->append_int64(length);
				break;
			}
		};

		const ssize_t length = payload->readable_bytes();
		if (length == 0) {
			LOG(WARNING) << "LengthHeaderCodec::encode Invalid length=0";
			return 0;
		} else if (length < min_payload() - checksum_length() || (max_payload() > 0 && length > max_payload())) {
			LOG(ERROR) << "LengthHeaderCodec::encode Invalid length=" << length
				<< ", max_payload=" << max_payload();
			return -1;
		}

		// FIXME: move bytes from |payload| to |buff|. (not copy)
		set_buff_length(length + checksum_length(), buff);
		buff->append(payload->begin_read(), length);
		
		// turn on/off crc32 checksum
		if (LIKELY(checksum_length() > 0)) {
			using crc32_func_t = uint32_t(*)(const char*, size_t);
			crc32_func_t crc32_func = nullptr;
			if (LIKELY(length > 60)) {
				crc32_func = Crc32c::crc32_long;
			} else {
				crc32_func = Crc32c::crc32_short;
			}

			uint32_t checksum = crc32_func(payload->begin_read(), length);
			buff->append_int32(checksum);
		}

		return 1;
	}

private:
	LENGTH_TYPE length_type()
	{
		return length_type_;
	}
	
	ssize_t max_payload()
	{
		return max_payload_;
	}
	
	ssize_t min_payload()
	{
		return checksum_length();
	}

	ssize_t checksum_length()
	{
		return checksum_length_;
	}

private:
	LENGTH_TYPE length_type_;
	ssize_t max_payload_;
	
	ssize_t checksum_length_;

	DISALLOW_COPY_AND_ASSIGN(LengthHeaderCodec);
};

}	// namespace annety

#endif  // ANT_CODEC_LENGTH_HEADER_CODEC_H_
