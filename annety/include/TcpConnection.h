// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_TCP_CONNECTION_H_
#define ANT_TCP_CONNECTION_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "Logging.h"
#include "EndPoint.h"
#include "NetBuffer.h"
#include "CallbackForward.h"
#include "containers/Any.h"

#include <string>
#include <atomic>
#include <memory>
#include <functional>

namespace annety
{
class Channel;
class EventLoop;
class SocketFD;
class NetBuffer;

// Connection wrapper of TCP protocol.
//
// This class owns the SelectableFD and Channel lifetime. Own lifetime 
// is held by Server (`ConnectionMap`) and users.
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop,
				const std::string& name,
				SelectableFDPtr sockfd,
				const EndPoint& localaddr,
				const EndPoint& peeraddr);
	
	void initialize();

	~TcpConnection();

	const std::string& name() const { return name_; }
	const EndPoint& local_addr() const { return local_addr_; }
	const EndPoint& peer_addr() const { return peer_addr_; }
	EventLoop* get_owner_loop() const { return owner_loop_; }

	// *Thread safe*
	bool connected() const { return state_ == kConnected; }
	bool disconnected() const { return state_ == kDisconnected; }
	bool is_reading() const { return reading_; };

	// *Thread safe*
	// Enable/Disable the read event. Is enabled by default.
	void start_read();
	void stop_read();

	// *Thread safe*
	void send(NetBuffer*);	// this one will swap data
	void send(NetBuffer&);	// this one will swap data
	void send(NetBuffer&&);
	void send(const NetBuffer*);
	void send(const NetBuffer&);
	void send(const void*, int);
	void send(const StringPiece&);

	// *Thread safe*
	// After the output buffer is sent, then handling shutdown 
	// the writable channel.
	void shutdown();
	// Don't care if the output buffer is sent, handling close 
	// the file descriptor (destory connection instance).
	void force_close();
	// Delay handling close the file descriptor (destory 
	// connection instance).
	void force_close_with_delay(double delay_s);

	// TCP opens the Nagle algorithm by default. Turn off it.
	void set_tcp_nodelay(bool on);

	// *Not thread safe*, but run in own loop thread.
	// Getter/Setter the connection context.
	void set_context(const containers::Any& context) 
	{
		context_ = context;
	}
	const containers::Any& get_context() const
	{
		return context_;
	}
	containers::Any* get_mutable_context()
	{
		return &context_;
	}

	// The callback function of the user moved from the `Server`.
	// *Not thread safe*, but run in own loop thread.
	void set_connect_callback(ConnectCallback cb)
	{
		connect_cb_ = std::move(cb);
	}
	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}
	void set_write_complete_callback(WriteCompleteCallback cb)
	{
		write_complete_cb_ = std::move(cb);
	}
	void set_high_water_mark_callback(HighWaterMarkCallback& cb, size_t high_water_mark)
	{
		high_water_mark_cb_ = std::move(cb);
		high_water_mark_ = high_water_mark;
	}
	void set_close_callback(CloseCallback cb)
	{
		close_cb_ = std::move(cb);
	}
	
	void connect_established();
	void connect_destroyed();

	NetBuffer* input_buffer();
	NetBuffer* output_buffer();

private:
	enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};

	void initialize_in_loop();

	void handle_read(TimeStamp);
	void handle_write();
	void handle_close();
	void handle_error();

	void send_in_loop(const StringPiece&);
	void send_in_loop(const void*, size_t);

	void shutdown_in_loop();
	void force_close_in_loop();

	void start_read_in_loop();
	void stop_read_in_loop();

	void set_state(StateE s) { state_ = s; }
	const char* state_to_string() const;

private:
	EventLoop* owner_loop_;
	const std::string name_;
	bool initilize_{false};

	std::atomic<bool> reading_{true};
	std::atomic<StateE> state_{kConnecting};

	const EndPoint local_addr_;
	const EndPoint peer_addr_;

	// connect socket
	SelectableFDPtr connect_socket_;
	std::unique_ptr<Channel> connect_channel_;

	// socket buffer
	std::unique_ptr<NetBuffer> input_buffer_;
	std::unique_ptr<NetBuffer> output_buffer_;

	// A connection's context.
	containers::Any context_;

	// User callback function.
	CloseCallback close_cb_;
	ConnectCallback connect_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;
	HighWaterMarkCallback high_water_mark_cb_;
	size_t high_water_mark_{64*1024*1024};

	DISALLOW_COPY_AND_ASSIGN(TcpConnection);
};

}	// namespace annety

#endif	// ANT_TCP_CONNECTION_H_
