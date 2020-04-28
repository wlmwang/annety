// By: wlmwang
// Date: Jun 30 2019

#ifndef ANT_TCP_CLIENT_H_
#define ANT_TCP_CLIENT_H_

#include "Macros.h"
#include "TcpConnection.h"
#include "CallbackForward.h"
#include "synchronization/MutexLock.h"

#include <map>
#include <string>
#include <atomic>
#include <memory>
#include <utility>
#include <functional>

namespace annety
{
class SocketFD;
class EndPoint;
class EventLoop;
class Connector;

// Client wrapper of TCP protocol.
//
// This class owns lifetime of Connector, it also storage the client 
// connection which has been connected.
class TcpClient : public std::enable_shared_from_this<TcpClient>
{
public:
	using ConnectorPtr = std::shared_ptr<Connector>;
	
	~TcpClient();

	const std::string& name() const { return name_;}
	EventLoop* get_own_loop() const { return owner_loop_;}

	void enable_retry()
	{
		retry_.store(true, std::memory_order_relaxed);
	}
	void disable_retry()
	{
		retry_.store(false, std::memory_order_relaxed);
	}
	bool retry() const
	{
		return retry_.load(std::memory_order_relaxed);
	}

	// For connect/disconnect, it maybe connect retry if enable.
	void connect();
	void disconnect();
	
	// Disconnect and stop connect retry.
	void stop();

	TcpConnectionPtr connection() const
	{
		AutoLock locked(lock_);
		return connection_;
	}

	// User registered callback functions.

	// *Not thread safe*, but usually be called before connect().
	void set_connect_callback(ConnectCallback cb)
	{
		connect_cb_ = std::move(cb);
	}
	// *Not thread safe*, but usually be called before connect().
	void set_close_callback(CloseCallback cb)
	{
		close_cb_ = std::move(cb);
	}
	// *Not thread safe*, but usually be called before connect().
	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}
	// *Not thread safe*, but usually be called before connect().
	void set_write_complete_callback(WriteCompleteCallback cb)
	{
		write_complete_cb_ = std::move(cb);
	}

private:
	// *Not thread safe*, but run in own loop thread.
	void new_connection(SelectableFDPtr&& sockfd, const EndPoint& peeraddr);

	// *Not thread safe*, but run in own loop thread.
	void remove_connection(const TcpConnectionPtr&);

	// *Not thread safe*, but run in own loop thread.
	void connect_destroyed();

	// *Not thread safe*, but run in own loop thread.
	void handle_retry();

	TcpClient(EventLoop* loop,
		const EndPoint& addr,
		const std::string& name = "a-crv");
	
	void initialize();

	friend TcpClientPtr make_tcp_client(EventLoop* loop, 
		const EndPoint& addr,
		const std::string& name);

private:
	EventLoop* owner_loop_;
	const std::string name_;
	const std::string ip_port_;
	bool initilize_{false};
	
	std::atomic<bool> connect_{false};
	std::atomic<bool> retry_{false};

	// Connect the processor.
	ConnectorPtr connector_;
	
	// Client connection.
	mutable MutexLock lock_;
	int next_conn_id_{1};
	TcpConnectionPtr connection_;

	// User registered callback functions.
	ConnectCallback connect_cb_;
	CloseCallback close_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;

	DISALLOW_COPY_AND_ASSIGN(TcpClient);
};

}	// namespace annety

#endif	// ANT_TCP_CLIENT_H_
