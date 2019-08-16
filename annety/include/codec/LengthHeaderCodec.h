// Refactoring: Anny Wang
// Date: Aug 15 2019

#ifndef ANT_CODEC_LENGTH_HEADER_CODEC_H_
#define ANT_CODEC_LENGTH_HEADER_CODEC_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "TcpConnection.h"
#include "CallbackForward.h"
#include "strings/StringPiece.h"

#include <functional>
#include <utility>

namespace annety
{
// stream protocol:
// struct byte_stream
// {
// 	uint32_t length;
// 	char body[0];
// }

class LengthHeaderCodec
{
public:
	enum PACKAGE_LENGTH_TYPE
	{
		_8	= 8,
		_16	= 16,
		_32	= 32,
		_64	= 64,
	};

	explicit LengthHeaderCodec(PACKAGE_LENGTH_TYPE length_type, int64_t max_length = 0) 
		: package_length_type_(length_type)
		, package_max_length_(max_length)
	{
		// CHECK(length_type >= _8 && length_type < _64);
	}

	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}

	void message_callback(const TcpConnectionPtr& conn, NetBuffer* buff, TimeStamp time)
	{
		int rt = decode(buff);
		switch (rt) {
		case 1:
			if (message_cb_) {
				message_cb_(conn, input_package_buff_, time);
			}
			break;
		case -1:
			conn->shutdown();	// FIXME: disable reading
		}
	}

	void send_callback(const TcpConnectionPtr& conn, const NetBuffer* buff)
	{
		int rt = encode(buff);
		switch (rt) {
		case 1:
			conn->send(output_package_buff_);
			break;
		case -1:
			LOG(ERROR) << "LengthHeaderCodec::send_callback Invalid buff " << rt;
		}
	}

	virtual int decode(NetBuffer* buff)
	{
		auto get_package_length = [this] (const NetBuffer* buff) {
			int64_t length = -1;
			
			switch (package_length_type_) {
			case _8:
				length = buff->peek_int8();
				break;

			case _16:
				length = buff->peek_int16();
				break;

			case _32:
				length = buff->peek_int32();
				break;

			case _64:
				length = buff->peek_int64();
				break;
			}

			return length;
		};

		while (buff->readable_bytes() >= package_length_type_) {
			const int64_t length = get_package_length(buff);
			if (length < 0 || (package_max_length_ > 0 && length > package_max_length_)) {
				LOG(ERROR) << "LengthHeaderCodec::decode Invalid length=" << length;
				return -1;
			} else if (buff->readable_bytes() >= length + package_length_type_) {
				buff->has_read(package_length_type_);
				
				// input_package_buff_.append(buff->begin_read(), length);
				return 1;
			} else {
				break;
			}
		}

		// again
		return 0;
	}
	
	virtual int encode(const StringPiece& mesg)
	{
		auto set_package_length = [] (NetBuffer* buff, const StringPiece& mesg) {
			switch (package_length_type_) {
			case _8:
				buff->append_int8(mesg.size());
				break;

			case _16:
				buff->append_int16(mesg.size());
				break;

			case _32:
				buff->append_int32(mesg.size());
				break;

			case _64:
				buff->append_int64(mesg.size());
				break;
			}
		};

		if (mesg.size() <= 0 || (package_max_length_ > 0 && length > package_max_length_)) {
			// LOG(ERROR) << "LengthHeaderCodec::encode Invalid length " << mesg.size();
			return -1;
		}

		set_package_length(&output_package_buff_, mesg);
		output_package_buff_.append(mesg.data(), mesg.size());

		return 1;
	}

private:
	PACKAGE_LENGTH_TYPE package_length_type_;
	int64_t package_max_length_;
	
	NetBuffer input_package_buff_;
	NetBuffer output_package_buff_;
	
	MessageCallback message_cb_;

	DISALLOW_COPY_AND_ASSIGN(LengthHeaderCodec);
};

}	// namespace annety

#endif  // ANT_CODEC_LENGTH_HEADER_CODEC_H_
