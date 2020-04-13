// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_TCP_SERVER_H_
#define ANT_TCP_SERVER_H_

#include "Macros.h"
#include "TcpConnection.h"
#include "CallbackForward.h"

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
class Acceptor;
class EventLoop;
class EventLoopPool;

// Server wrapper of TCP protocol.
// It supports single-thread and thread-pool model.
//
// This class owns lifetime of Acceptor, it also storages all TcpConnection 
// instances which has been connected.
class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	TcpServer(EventLoop* loop,
			  const EndPoint& addr,
			  const std::string& name = "a-srv",
			  bool reuse_port = false);

	~TcpServer();
	
	void initialize();
	void set_thread_num(int num_threads);

	void start();

	const std::string& ip_port() const { return ip_port_; }
	const std::string& name() const { return name_; }
	EventLoop* get_owner_loop() const { return owner_loop_; }
	
	// The following interfaces are usually be called before start()
	// *Not thread safe*
	void set_connect_callback(ConnectCallback cb)
	{
		connect_cb_ = std::move(cb);
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
	// *Not thread safe*
	void set_thread_init_callback(ThreadInitCallback cb)
	{
		thread_init_cb_ = std::move(cb);
	}

private:
	// *Thread safe*
	void remove_connection(const TcpConnectionPtr& conn);
	// *Not thread safe*, but run in own loop thread.
	void remove_connection_in_loop(const TcpConnectionPtr& conn);
	
	// *Not thread safe*, but run in own loop thread.
	void new_connection(SelectableFDPtr&& sockfd, const EndPoint& peeraddr);

private:
	using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

	EventLoop* owner_loop_{nullptr};
	const std::string name_;
	const std::string ip_port_;

	bool initilize_{false};
	
	// ATOMIC_FLAG_INIT is macro
	std::atomic_flag started_ = ATOMIC_FLAG_INIT;

	// Acceptor of listen-socket Server.
	std::unique_ptr<Acceptor> acceptor_;
	
	// ThreadPool of event loops.
	std::unique_ptr<EventLoopPool> loop_pool_;

	// User registered callback functions.
	ConnectCallback connect_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;
	ThreadInitCallback thread_init_cb_;

	// Client connections pool (std::map).
	std::atomic<int> next_conn_id_{1};
	ConnectionMap connections_;

	DISALLOW_COPY_AND_ASSIGN(TcpServer);
};

}	// namespace annety

#endif	// ANT_TCP_SERVER_H_
