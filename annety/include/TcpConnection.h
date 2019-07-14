// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TCP_CONNECTION_H_
#define ANT_TCP_CONNECTION_H_

#include "Macros.h"
#include "EndPoint.h"
#include "Times.h"
#include "CallbackForward.h"
#include "containers/Any.h"

#include <string>
#include <memory>
#include <functional>

namespace annety
{
class Channel;
class EventLoop;
class SocketFD;

// Wrapper client with TCP protocol
//
// this class owns life-time of connect-socket and connect-channel,
// and itself life-time was hold by TcpServer and users
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop,
				const std::string& name,
				SelectableFDPtr sockfd,
				const EndPoint& localaddr,
				const EndPoint& peeraddr);
	~TcpConnection();

	EventLoop* get_owner_loop() const { return owner_loop_; }
	const std::string& name() const { return name_; }
	const EndPoint& local_addr() const { return local_addr_; }
	const EndPoint& peer_addr() const { return peer_addr_; }

	// not thread safe.
	bool connected() const { return state_ == kConnected; }
	bool disconnected() const { return state_ == kDisconnected; }

	// not thread safe.
	void send(const void* message, int len);
	void send(const StringPiece& message);
	void send(NetBuffer&& message);
	void send(NetBuffer* message);  // this one will swap data
	void shutdown(); // NOT thread safe, no simultaneous calling
	void force_close();
	void force_close_with_delay(double delay_s);

	void set_tcp_nodelay(bool on);

	// reading or not
	void start_read();
	void stop_read();
	bool is_reading() const { return reading_; };

	void set_context(const Any& context) 
	{
		context_ = context;
	}
	const Any& get_context() const
	{
		return context_;
	}
	Any* get_mutable_context()
	{
		return &context_;
	}

	void set_connect_callback(const ConnectCallback& cb)
	{
		connect_cb_ = cb;
	}
	void set_message_callback(const MessageCallback& cb)
	{
		message_cb_ = cb;
	}
	void set_write_complete_callback(const WriteCompleteCallback& cb)
	{
		write_complete_cb_ = cb;
	}
	void set_high_water_mark_callback(const HighWaterMarkCallback& cb, size_t high_water_mark)
	{
		high_water_mark_cb_ = cb;
		high_water_mark_ = high_water_mark;
	}

	void set_close_callback(const CloseCallback& cb)
	{
		close_cb_ = cb;
	}
	
	NetBuffer* input_buffer();
	NetBuffer* output_buffer();

	void connect_established();
	void connect_destroyed();

private:
	enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting};

	void handle_read(Time recv_tm);
	void handle_write();
	void handle_close();
	void handle_error();

	void send_in_loop(const StringPiece& message);
	void send_in_loop(const void* message, size_t len);
	void shutdown_in_loop();
	void force_close_in_loop();
	void start_read_in_loop();
	void stop_read_in_loop();

	void set_state(StateE s) { state_ = s; }
	const char* state_to_string() const;

private:
	EventLoop* owner_loop_;
	const std::string name_;
	bool reading_{true};
	StateE state_{kConnecting};

	const EndPoint local_addr_;
	const EndPoint peer_addr_;

	// connect socket
	SelectableFDPtr connect_socket_;
	std::unique_ptr<Channel> connect_channel_;

	CloseCallback close_cb_;

	// user callback function
	ConnectCallback connect_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;
	HighWaterMarkCallback high_water_mark_cb_;
	size_t high_water_mark_{64*1024*1024};

	std::unique_ptr<NetBuffer> input_buffer_;
	std::unique_ptr<NetBuffer> output_buffer_;

	// a connection's context
	Any context_;

	DISALLOW_COPY_AND_ASSIGN(TcpConnection);
};

}	// namespace annety

#endif	// ANT_TCP_CONNECTION_H_
