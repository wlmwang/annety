// By: wlmwang
// Date: Oct 12 2019

#ifndef ANT_PROTOBUF_PROTOBUF_CODEC_H
#define ANT_PROTOBUF_PROTOBUF_CODEC_H

#include "build/CompilerSpecific.h"
#include "Macros.h"
#include "Logging.h"
#include "ByteOrder.h"
#include "Crc32c.h"
#include "codec/Codec.h"

#include <string>
#include <utility>
#include <functional>
#include <google/protobuf/message.h>

namespace annety
{
namespace
{
BEFORE_MAIN_EXECUTOR() { GOOGLE_PROTOBUF_VERIFY_VERSION;}

inline uint32_t peek_uint32(const char* buff)
{
	uint32_t be32 = 0;
	::memcpy(&be32, buff, sizeof be32);
	return net_to_host32(be32);
}
}	// namespace anonymous

namespace
{
const std::string kNoErrorStr = "NoError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";
}	// namespace anonymous

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

using ProtobufMessageCallback = 
		std::function<void(const TcpConnectionPtr&, const MessagePtr&, TimeStamp)>;

// A codec that handle protobuf bytes of the following struct's streams:
// struct streams __attribute__ ((__packed__))
// {
//   int32_t  length;
//   int32_t  nameLen;
//   char     typeName[nameLen];	// c-style string
//   char     protobufData[length-nameLen-8];
//   int32_t  checkSum;				// crc32 of nameLen, typeName and protobufData
// }
//
// A Simple Fixed Head Protobuf Codec with Length and protobuf::message::TypeName
class ProtobufCodec : public Codec
{
public:
	enum LENGTH_TYPE
	{
		kLengthType8	= 1,
		kLengthType16	= 2,
		kLengthType32	= 4,
		kLengthType64	= 8,
	};

	enum ERROR_CODE
	{
		kNoError = 0,
		kInvalidNameLen,
		kUnknownMessageType,
		kParseError,
	};

	using ErrorCallback = 
			std::function<void(const TcpConnectionPtr&, NetBuffer*, TimeStamp, ERROR_CODE)>;

	ProtobufCodec(EventLoop* loop, ProtobufMessageCallback cb) 
		: ProtobufCodec(loop, std::move(cb), ProtobufCodec::error_callback) {}

	ProtobufCodec(EventLoop* loop, ProtobufMessageCallback pmcb, ErrorCallback ecb) 
		: Codec(loop), dispatch_cb_(std::move(pmcb)), error_cb_(std::move(ecb))
	{
		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;

		set_message_callback(std::bind(&ProtobufCodec::parse, this, _1, _2, _3));
	}
	
	void send(const TcpConnectionPtr& conn, const google::protobuf::Message& mesg)
	{
		NetBuffer payload;
		if (serialize(mesg, &payload)) {
			Codec::send(conn, &payload);
		}
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
			if (length < min_payload() || length > max_payload()) {
				LOG(ERROR) << "ProtobufCodec::decode Invalid length=" << length
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
					LOG(ERROR) << "ProtobufCodec::decode Invalid checksum=" << checksum
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
		} else if (length < min_payload() - checksum_length() || length > max_payload()) {
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
	bool serialize(const google::protobuf::Message&, NetBuffer*);
	void parse(const TcpConnectionPtr&, NetBuffer*, TimeStamp);

	static ERROR_CODE parse_mesg(NetBuffer*, MessagePtr&);
	static google::protobuf::Message* generate_mesg(const std::string&);

	static void error_callback(const TcpConnectionPtr&, NetBuffer*, TimeStamp, ERROR_CODE);
	static const std::string& to_errstr(ERROR_CODE errorCode);

	// sizeof(int32_t)
	static constexpr LENGTH_TYPE length_type()
	{
		return kLengthType32;
	}
	
	static constexpr ssize_t header_length()
	{
		return sizeof(uint32_t);
	}

	// same as codec_stream.h kDefaultTotalBytesLimit
	static constexpr ssize_t max_payload()
	{
		return 64*1024*1024;
	}

	// nameLen >= 2
	static constexpr ssize_t min_payload()
	{
		return header_length() + 2 + checksum_length();
	}

	// return sizeof(uint32_t)
	// turn on/off crc32 checksum
	static constexpr ssize_t checksum_length()
	{
		return sizeof(uint32_t);
	}

private:
	ProtobufMessageCallback dispatch_cb_;
	ErrorCallback error_cb_;

	DISALLOW_COPY_AND_ASSIGN(ProtobufCodec);
};

bool ProtobufCodec::serialize(const google::protobuf::Message& mesg, NetBuffer* payload)
{
	DCHECK(mesg.IsInitialized());
	DCHECK(!payload->readable_bytes());

	// append typenamelen + typename
	const std::string& type_name = mesg.GetTypeName();
	int32_t nameLen = static_cast<int32_t>(type_name.size()+1); // c-style string
	payload->append_int32(nameLen);
	payload->append(type_name.data(), nameLen);

	// append message
	int size = mesg.ByteSize();
	payload->ensure_writable_bytes(size);
	mesg.SerializeToArray(payload->begin_write(), size);
	payload->has_written(size);

	DCHECK(payload->readable_bytes() == sizeof(nameLen) + nameLen + size);

	return true;
}

void ProtobufCodec::parse(const TcpConnectionPtr& conn, NetBuffer* payload, TimeStamp receive)
{
	MessagePtr mesg;
	ERROR_CODE err = parse_mesg(payload, mesg);

	if (err == kNoError && mesg) {
		if (dispatch_cb_) {
			dispatch_cb_(conn, mesg, receive);
		} else {
			LOG(WARNING) << "ProtobufCodec::parse no dispatch callback";
		}
	} else {
		error_cb_(conn, payload, receive, err);
	}
}

ProtobufCodec::ERROR_CODE ProtobufCodec::parse_mesg(NetBuffer* payload, MessagePtr& mesg)
{
	DCHECK(payload->readable_bytes() > 0);

	ERROR_CODE err = kNoError;

	const size_t nameLen = payload->read_int32();
	if (nameLen >= 2 && nameLen <= payload->readable_bytes()) {
		// type name (-1 because of the index is 0 offset)
		std::string name(payload->begin_read(), payload->begin_read() + nameLen - 1);

		// create (prototype) message object from typename
		mesg.reset(generate_mesg(name));
		if (mesg) {
			// parse protobuf from payload
			const char* data = payload->begin_read() + nameLen;
			int32_t dataLen = payload->readable_bytes() - nameLen;
			if (mesg->ParseFromArray(data, dataLen)) {
				err = kNoError;
			} else {
				err = kParseError;
			}
		} else {
			err = kUnknownMessageType;
		}
	} else {
		err = kInvalidNameLen;
	}

	return err;
}

google::protobuf::Message* ProtobufCodec::generate_mesg(const std::string& name)
{
	using namespace google::protobuf;
	
	Message* mesg = nullptr;

	const Descriptor* desc = DescriptorPool::generated_pool()->FindMessageTypeByName(name);
	if (desc) {
		const Message* prototype = MessageFactory::generated_factory()->GetPrototype(desc);
		if (prototype) {
			mesg = prototype->New();
		}
	}

	return mesg;
}

void ProtobufCodec::error_callback(const TcpConnectionPtr& conn, NetBuffer*, TimeStamp, ERROR_CODE err)
{
	LOG(ERROR) << "ProtobufCodec::error_callback - " << to_errstr(err);
	if (conn && conn->connected()) {
		conn->shutdown();
	}
}

const std::string& ProtobufCodec::to_errstr(ERROR_CODE errorCode)
{
	switch (errorCode)
	{
	case kNoError:
		return kNoErrorStr;
	case kInvalidNameLen:
		return kInvalidNameLenStr;
	case kUnknownMessageType:
		return kUnknownMessageTypeStr;
	case kParseError:
		return kParseErrorStr;
	default:
		return kUnknownErrorStr;
	}
}

}	// namespace annety

#endif  // ANT_PROTOBUF_PROTOBUF_CODEC_H
