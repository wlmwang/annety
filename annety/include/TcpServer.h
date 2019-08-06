// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TCP_SERVER_H_
#define ANT_TCP_SERVER_H_

#include "Macros.h"
#include "TcpConnection.h"
#include "CallbackForward.h"

#include <map>
#include <string>
#include <memory>
#include <atomic>
#include <functional>

namespace annety
{
class SocketFD;
class EndPoint;
class Acceptor;
class EventLoop;
class EventLoopPool;

// Wrapper server with TCP protocol
// It supports single-thread and thread-pool models
//
// This class owns life-time of Acceptor, and also storage all TcpConnections 
// which has been connected itself.
class TcpServer
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	enum Option
	{
		kNoReusePort,
		kReusePort,
	};

	TcpServer(EventLoop* loop,
			  const EndPoint& addr,
			  const std::string& name = "a-srv",
			  Option option = kNoReusePort);

	~TcpServer();
	
	const std::string& ip_port() const { return ip_port_; }
	const std::string& name() const { return name_; }
	EventLoop* get_owner_loop() const { return owner_loop_; }

	void start();

	void set_thread_num(int num_threads);
	
	void set_thread_init_callback(const ThreadInitCallback& cb)
	{
		thread_init_cb_ = cb;
	}

	// The following interfaces are usually be called before start()
	// *Not thread safe*
	void set_connect_callback(const ConnectCallback& cb)
	{
		connect_cb_ = cb;
	}
	// *Not thread safe*
	void set_message_callback(const MessageCallback& cb)
	{
		message_cb_ = cb;
	}
	// *Not thread safe*
	void set_write_complete_callback(const WriteCompleteCallback& cb)
	{
		write_complete_cb_ = cb;
	}

private:
	// *Not thread safe*, but in loop
	void new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr);
	// *Thread safe*
	void remove_connection(const TcpConnectionPtr& conn);
	// *Not thread safe*, but in loop
	void remove_connection_in_loop(const TcpConnectionPtr& conn);

private:
	using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

	EventLoop* owner_loop_{nullptr};
	const std::string name_;
	const std::string ip_port_;

	// ATOMIC_FLAG_INIT is macros
	std::atomic_flag started_ = ATOMIC_FLAG_INIT;

	// accept connect socket
	std::unique_ptr<Acceptor> acceptor_;
	
	// worker thread pool
	std::unique_ptr<EventLoopPool> loop_pool_;

	// user callback functions
	ConnectCallback connect_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;
	ThreadInitCallback thread_init_cb_;

	std::atomic<int> next_conn_id_{1};
	
	// all connections pool(map)
	ConnectionMap connections_;

	DISALLOW_COPY_AND_ASSIGN(TcpServer);
};

}	// namespace annety

#endif	// ANT_TCP_SERVER_H_
