// By: wlmwang
// Date: Aug 15 2019

#ifndef ANT_CODEC_CODEC_H_
#define ANT_CODEC_CODEC_H_

#include "Macros.h"
#include "Logging.h"
#include "TimeStamp.h"
#include "TcpConnection.h"
#include "CallbackForward.h"
#include "EventLoop.h"

#include <functional>
#include <utility>

namespace annety
{
class Codec
{
public:
	explicit Codec(EventLoop* loop) : owner_loop_(loop)
	{
		CHECK(loop);
	}

	virtual ~Codec() {}

	// Decode payload from |buff| to |payload|
	// NOTE: You must be remove the read bytes from |buff| when decode success
	// Returns:
	//   -1  decode error, going to close connection
	//    1  decode success, going to call message callback
	//    0  decode incomplete, continues to read more data
	virtual int decode(NetBuffer* buff, NetBuffer* payload) = 0;

	// Encode stream bytes from |payload| to |buff|
	// NOTE: You must be remove the sent bytes from |payload| when encode success
	// Returns:
	//   -1  encode error, going to close connection
	//    1  encode success, going to send data to peer
	//    0  encode incomplete, continues to send more data
	virtual int encode(NetBuffer* payload, NetBuffer* buff) = 0;
	
	// *Not thread safe*
	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}

	// *Not thread safe*
	// Make sure run in own loop
	void decode_read(const TcpConnectionPtr& conn, NetBuffer* buff, TimeStamp time)
	{
		owner_loop_->check_in_own_loop();

		int rt = 0;
		do {
			NetBuffer payload;
			rt = decode(buff, &payload);

			if (rt == 1) {
				if (message_cb_) {
					message_cb_(conn, &payload, time);
				} else {
					LOG(WARNING) << "LengthHeaderCodec::message_callback no message callback";
				}
			} else if (rt == -1) {
				conn->shutdown();
				LOG(ERROR) << "LengthHeaderCodec::message_callback Invalid buff, rt=" << rt;
			}
		} while (rt == 1);
	}

	// *Thread safe*
	void endcode_send(const TcpConnectionPtr& conn, NetBuffer* payload)
	{
		int rt = 0;
		do {
			NetBuffer buff;
			rt = encode(payload, &buff);

			if (rt == 1) {
				conn->send(&buff);
			} else if (rt == -1) {
				conn->shutdown();
				LOG(ERROR) << "LengthHeaderCodec::send_callback Invalid buff, rt=" << rt;
			}
		} while (0);
	}

protected:
	EventLoop* owner_loop_;

	MessageCallback message_cb_;

	DISALLOW_COPY_AND_ASSIGN(Codec);
};

}	// namespace annety

#endif	// ANT_CODEC_CODEC_H_
