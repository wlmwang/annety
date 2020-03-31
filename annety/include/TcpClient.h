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
#include <memory>
#include <utility>
#include <atomic>
#include <functional>

namespace annety
{
class SocketFD;
class EndPoint;
class EventLoop;
class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

// Tcp client wrapper of TCP protocol
class TcpClient : public std::enable_shared_from_this<TcpClient>
{
public:
	TcpClient(EventLoop* loop,
			const EndPoint& addr,
			const std::string& name = "a-crv");
	
	~TcpClient();  // force out-line dtor, for std::unique_ptr members.

	void initialize();

	EventLoop* get_own_loop() const { return owner_loop_;}
	const std::string& name() const { return name_;}
	
	bool retry() const { return retry_;}
	void enable_retry() { retry_ = true;}
	void disable_retry() { retry_ = false;}

	// for connection
	void connect();
	void disconnect();

	// for connector_ quit, and wait Four-Way Wavehand to disconnect()
	void stop();

	TcpConnectionPtr connection() const
	{
		AutoLock locked(lock_);
		return connection_;
	}

	// The following interfaces are usually be called before start()
	// *Not thread safe*
	void set_connect_callback(ConnectCallback cb)
	{
		connect_cb_ = std::move(cb);
	}
	// *Not thread safe*
	void set_error_callback(ErrorCallback cb)
	{
		error_cb_ = std::move(cb);
	}
	// *Not thread safe*
	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}
	// *Not thread safe*
	void set_write_complete_callback(WriteCompleteCallback cb)
	{
		write_complete_cb_ = std::move(cb);
	}

private:
	// *Not thread safe*, but in loop
	void remove_connection(const TcpConnectionPtr& conn);

	// *Not thread safe*, but in loop
	void new_connection(SelectableFDPtr&& sockfd, const EndPoint& peeraddr);

	// *Not thread safe*, but in loop
	void error_connect();

	void close_connection();

private:
	EventLoop* owner_loop_;
	const std::string name_;
	const std::string ip_port_;
	bool initilize_{false};
	
	ConnectorPtr connector_;	// avoid revealing Connector

	ErrorCallback error_cb_;
	ConnectCallback connect_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;

	bool retry_{false};		// atomic
	bool connect_{true};	// atomic
	
	std::atomic<int> next_conn_id_{1};

	mutable MutexLock lock_;
	TcpConnectionPtr connection_;
};

}	// namespace annety

#endif	// ANT_TCP_CLIENT_H_
