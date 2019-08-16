// Refactoring: Anny Wang
// Date: Aug 15 2019

#ifndef ANT_CODEC_LENGTH_HEADER_CODEC_H_
#define ANT_CODEC_LENGTH_HEADER_CODEC_H_

#include "Macros.h"
#include "Logging.h"
#include "TimeStamp.h"
#include "TcpConnection.h"
#include "CallbackForward.h"
#include "strings/StringPiece.h"

#include <functional>
#include <utility>

namespace annety
{
// Like stream protocol:
// 
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
		_8	= 1,
		_16	= 2,
		_32	= 4,
		_64	= 8,
	};

	explicit LengthHeaderCodec(PACKAGE_LENGTH_TYPE length_type, int64_t max_length = 0) 
		: package_length_type_(length_type)
		, package_max_length_(max_length)
	{
		// DCHECK(length_type >= _8 && length_type < _64);
	}

	virtual ~LengthHeaderCodec() {}

	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}

	PACKAGE_LENGTH_TYPE package_length_type()
	{
		return package_length_type_;
	}
	
	int64_t package_max_length()
	{
		return package_max_length_;
	}

	void message_callback(const TcpConnectionPtr& conn, NetBuffer* buff, TimeStamp time)
	{
		int rt = 0;
		do {
			input_package_mesg_.reset();
			rt = decode(buff, &input_package_mesg_);

			if (rt == 1) {
				// remove read messages
				buff->has_read(package_length_type() + input_package_mesg_.readable_bytes());

				if (message_cb_) {
					message_cb_(conn, &input_package_mesg_, time);
				} else {
					LOG(WARNING) << "LengthHeaderCodec::message_callback no message callback";
				}
			} else if (rt == -1) {
				conn->shutdown();
				LOG(ERROR) << "LengthHeaderCodec::message_callback Invalid buff, rt=" << rt;
			}
		} while (rt == 1);
	}

	void send_callback(const TcpConnectionPtr& conn, NetBuffer* buff)
	{
		int rt = 0;
		do {
			output_package_mesg_.reset();
			rt = encode(buff, &output_package_mesg_);

			if (rt == 1) {
				conn->send(&output_package_mesg_);
				// remove read messages
				buff->has_read_all();
			} else if (rt == -1) {
				conn->shutdown();
				LOG(ERROR) << "LengthHeaderCodec::send_callback Invalid buff, rt=" << rt;
			}
		} while (0);
	}

	// decode |buff| bytes to |package_mesg|
	virtual int decode(const NetBuffer* buff, NetBuffer* package_mesg)
	{
		auto get_package_length = [this] (const NetBuffer* buff) {
			int64_t length = -1;
			switch (package_length_type()) {
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

		int rt = 0;
		if (buff->readable_bytes() >= package_length_type()) {
			const int64_t length = get_package_length(buff);
			if (length < 0 || (package_max_length() > 0 && length > package_max_length())) {
				LOG(ERROR) << "LengthHeaderCodec::decode Invalid length=" << length
					<< ", package_max_length = " << package_max_length();
				rt = -1;
			} else if (buff->readable_bytes() >= length + package_length_type()) {
				// FIXME: only copy offset, but *Not thread safe*
				// copy body
				package_mesg->append(buff->begin_read() + package_length_type(), length);
				rt = 1;
			}
		}
		return rt;
	}
	
	// encode |buff| bytes to |package_mesg|
	virtual int encode(const NetBuffer* buff, NetBuffer* package_mesg)
	{
		auto set_package_length = [this] (const NetBuffer* buff, NetBuffer* package_mesg) {
			switch (package_length_type()) {
			case _8:
				package_mesg->append_int8(buff->readable_bytes());
				break;

			case _16:
				package_mesg->append_int16(buff->readable_bytes());
				break;

			case _32:
				package_mesg->append_int32(buff->readable_bytes());
				break;

			case _64:
				package_mesg->append_int64(buff->readable_bytes());
				break;
			}
		};

		const int64_t length = buff->readable_bytes();
		if (length == 0) {
			LOG(WARNING) << "LengthHeaderCodec::encode Invalid length=" << 0;
			return 0;
		} else if (length < 0 || (package_max_length() > 0 && length > package_max_length())) {
			LOG(ERROR) << "LengthHeaderCodec::encode Invalid length=" << length
				<< ", package_max_length = " << package_max_length();
			return -1;
		}

		// copy buff
		set_package_length(buff, package_mesg);
		package_mesg->append(buff->begin_read(), length);
		return 1;
	}

private:
	PACKAGE_LENGTH_TYPE package_length_type_;
	int64_t package_max_length_;
	
	NetBuffer input_package_mesg_;
	NetBuffer output_package_mesg_;
	
	MessageCallback message_cb_;

	DISALLOW_COPY_AND_ASSIGN(LengthHeaderCodec);
};

}	// namespace annety

#endif  // ANT_CODEC_LENGTH_HEADER_CODEC_H_
