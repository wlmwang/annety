// By: wlmwang
// Date: Oct 12 2019

#ifndef ANT_CODEC_PROTOBUF_CODEC_H
#define ANT_CODEC_PROTOBUF_CODEC_H

#include "Macros.h"
#include "Logging.h"
#include "codec/Codec.h"

#include <string>
#include <utility>
#include <functional>
#include <google/protobuf/message.h>

namespace annety
{
namespace
{
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
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
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
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
		kInvalidLength,
		kCheckSumError,
		kInvalidNameLen,
		kUnknownMessageType,
		kParseError,
	};

	using ErrorCallback = 
			std::function<void(const TcpConnectionPtr&, NetBuffer*, TimeStamp, ERROR_CODE)>;

	explicit ProtobufCodec(EventLoop* loop) : ProtobufCodec(loop, error_callback) {}

	ProtobufCodec(EventLoop* loop, ErrorCallback cb) 
		: Codec(loop), error_cb_(std::move(cb))
	{
		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;

		set_message_callback(std::bind(&ProtobufCodec::parse_payload, this, _1, _2, _3));
	}

	// *Not thread safe*
	void set_dispatch_callback(ProtobufMessageCallback cb)
	{
		dispatch_cb_ = std::move(cb);
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
				// FIXME: move bytes from |buff| to |payload|. (not copy)
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
		DCHECK(!!buff && !!payload);
		
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

		// FIXME: move bytes from |payload| to |buff|. (not copy)
		set_buff_length(payload, buff);
		buff->append(payload->begin_read(), length);
		payload->has_read_all();

		return 1;
	}

	void endcode_send(const TcpConnectionPtr& conn, const google::protobuf::Message& mesg)
	{
		NetBuffer payload;
		if (serialize_payload(mesg, &payload)) {
			Codec::endcode_send(conn, &payload);
		}
	}

private:
	bool serialize_payload(const google::protobuf::Message&, NetBuffer*);
	void parse_payload(const TcpConnectionPtr&, NetBuffer*, TimeStamp);

	static ERROR_CODE parse(NetBuffer*, MessagePtr&);
	static google::protobuf::Message* generate_message(const std::string&);

	static void error_callback(const TcpConnectionPtr&, NetBuffer*, TimeStamp, ERROR_CODE);
	static const std::string& to_errstr(ERROR_CODE errorCode);

	// sizeof(int32_t)
	static LENGTH_TYPE length_type()
	{
		return kLengthType32;
	}
	
	// same as codec_stream.h kDefaultTotalBytesLimit
	static ssize_t max_payload()
	{
		return 64*1024*1024;
	}

	// nameLen >= 2
	static ssize_t min_payload()
	{
		return header_length() + 2 + checksum_length();
	}
	
	static ssize_t header_length()
	{
		return sizeof(int32_t);
	}

	// return sizeof(int32_t);
	static ssize_t checksum_length()
	{
		return 0;
	}

private:
	ProtobufMessageCallback dispatch_cb_;
	ErrorCallback error_cb_;

	DISALLOW_COPY_AND_ASSIGN(ProtobufCodec);
};

bool ProtobufCodec::serialize_payload(const google::protobuf::Message& mesg, NetBuffer* payload)
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

void ProtobufCodec::parse_payload(const TcpConnectionPtr& conn, NetBuffer* payload, TimeStamp receive)
{
	MessagePtr mesg;
	ERROR_CODE err = parse(payload, mesg);

	if (err == kNoError && mesg) {
		if (dispatch_cb_) {
			dispatch_cb_(conn, mesg, receive);
		} else {
			LOG(WARNING) << "ProtobufCodec::parse_payload no dispatch callback";
		}
	} else {
		error_cb_(conn, payload, receive, err);
	}
}

ProtobufCodec::ERROR_CODE ProtobufCodec::parse(NetBuffer* payload, MessagePtr& mesg)
{
	DCHECK(payload->readable_bytes() > 0);

	ERROR_CODE err = kNoError;

	const size_t nameLen = payload->read_int32();
	if (nameLen >= 2 && nameLen <= payload->readable_bytes() - checksum_length()) {
		// type name (-1 because of the index is 0 offset)
		std::string name(payload->begin_read(), payload->begin_read() + nameLen - 1);

		// create (prototype) message object from typename
		mesg.reset(generate_message(name));
		if (mesg) {
			// parse protobuf from payload
			const char* data = payload->begin_read() + nameLen;
			int32_t dataLen = payload->readable_bytes() - nameLen - checksum_length();
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

google::protobuf::Message* ProtobufCodec::generate_message(const std::string& name)
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
	case kInvalidLength:
		return kInvalidLengthStr;
	case kCheckSumError:
		return kCheckSumErrorStr;
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

#endif  // ANT_CODEC_PROTOBUF_CODEC_H