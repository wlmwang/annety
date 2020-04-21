// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_TCP_SERVER_H_
#define ANT_TCP_SERVER_H_

#include "Macros.h"
#include "TcpConnection.h"
#include "CallbackForward.h"

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
class Acceptor;
class EventLoop;
class EventLoopPool;

// Server wrapper of TCP protocol.
// It supports single-thread and multi-thread model.
//
// This class owns lifetime of Acceptor, it also storages all client 
// connections pool which has been connected.
class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;
	
	~TcpServer();

	// Starts the server if it's not listenning.
	void start();

	const std::string& name() const { return name_; }
	const std::string& ip_port() const { return ip_port_; }

	// Turn on the server to become a multi-threaded model.
	// `num_threads` is the number of threads for handling input.
	// - 0 means all I/O in owner_loop_'s thread, no thread will 
	// 	 be created, this is the default value.
	// - 1 means all I/O in another thread.
	// - N means a thread pool with N threads, new connections
	//   are assigned on a round-robin basis.
	//
	// NOTICE: Accepts new connection always in owner_loop_'s thread.
	//
	// *Not thread safe*, but usually be called before start().
	void set_thread_num(int num_threads);
	// *Not thread safe*, but usually be called before start().
	void set_thread_init_callback(ThreadInitCallback cb)
	{
		thread_init_cb_ = std::move(cb);
	}

	// User registered callback functions.

	// *Not thread safe*, but usually be called before start().
	void set_connect_callback(ConnectCallback cb)
	{
		connect_cb_ = std::move(cb);
	}
	void set_close_callback(CloseCallback cb)
	{
		close_cb_ = std::move(cb);
	}
	// *Not thread safe*, but usually be called before start().
	void set_message_callback(MessageCallback cb)
	{
		message_cb_ = std::move(cb);
	}
	// *Not thread safe*, but usually be called before start().
	void set_write_complete_callback(WriteCompleteCallback cb)
	{
		write_complete_cb_ = std::move(cb);
	}

private:
	// *Not thread safe*, but run in own loop thread.
	void new_connection(SelectableFDPtr&& peerfd, const EndPoint& peeraddr);

	// *Thread safe*, for Connection.
	void remove_connection(const TcpConnectionPtr&);
	// *Not thread safe*, but run in own loop thread.
	void remove_connection_in_loop(const TcpConnectionPtr&);

	TcpServer(EventLoop* loop,
		const EndPoint& addr,
		const std::string& name = "a-srv",
		bool reuse_port = false);

	void initialize();

	friend TcpServerPtr make_tcp_server(EventLoop* loop, 
		const EndPoint& addr, 
		const std::string& name, 
		bool reuse_port);

private:
	using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

	EventLoop* owner_loop_{nullptr};
	const std::string name_;
	const std::string ip_port_;
	bool initilize_{false};
	
	// ATOMIC_FLAG_INIT is macro
	std::atomic_flag started_ = ATOMIC_FLAG_INIT;

	// Accept the processor.
	std::unique_ptr<Acceptor> acceptor_;
	
	// ThreadPool of event loops.
	std::unique_ptr<EventLoopPool> workers_;

	// Client connections pool (std::map).
	int next_conn_id_{1};
	ConnectionMap connections_;

	// User registered callback functions.
	ConnectCallback connect_cb_;
	CloseCallback close_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;
	ThreadInitCallback thread_init_cb_;

	DISALLOW_COPY_AND_ASSIGN(TcpServer);
};

}	// namespace annety

#endif	// ANT_TCP_SERVER_H_
