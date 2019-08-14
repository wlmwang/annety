// Refactoring: Anny Wang
// Date: Aug 03 2019

#ifndef EXAMPLES_ASIO_CHAT_LENGTH_HEADER_CODEC_H
#define EXAMPLES_ASIO_CHAT_LENGTH_HEADER_CODEC_H

#include "TcpConnection.h"
#include "CallbackForward.h"

#include <string>

class LengthHeaderCodec
{
const static size_t kHeaderLen = sizeof(int32_t);

public:
	using PacketMessageCallback = std::function<
			void(const annety::TcpConnectionPtr& conn,
				const std::string& message, annety::TimeStamp time
		)>;

	explicit LengthHeaderCodec(const PacketMessageCallback& cb)
		: packet_message_callback_(cb) {}

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time)
	{
		// kHeaderLen == 4
		while (buf->readable_bytes() >= kHeaderLen) {
			const int32_t len = buf->peek_int32();
			if (len > 65536 || len < 0) {
				LOG(ERROR) << "Invalid length " << len;
				conn->shutdown();	// FIXME: disable reading
				break;
			} else if (buf->readable_bytes() >= len + kHeaderLen) {
				buf->has_read_int32();

				// message body
				std::string message{buf->taken_as_string()};
				if (packet_message_callback_) {
					packet_message_callback_(conn, message, time);
				}
			} else {
				break;
			}
		}
	}

	void send(const annety::TcpConnectionPtr& conn, const std::string& message)
	{
		annety::NetBuffer buf;
		buf.append_int32(message.size());
		buf.append(message.data(), message.size());
		conn->send(&buf);
	}

private:
	PacketMessageCallback packet_message_callback_;
};

#endif  // EXAMPLES_ASIO_CHAT_LENGTH_HEADER_CODEC_H
