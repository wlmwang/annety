// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TCP_SERVER_H_
#define ANT_TCP_SERVER_H_

#include "Macros.h"
#include "CallbackForward.h"

#include <map>
#include <string>
#include <memory>
#include <atomic>

namespace annety
{
class SocketFD;
class EndPoint;
class Acceptor;
class EventLoop;

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
			  const std::string& name = "annety-srv",
			  Option option = kNoReusePort);

	~TcpServer();

	// not thread safe.
	void setConnectionCallback(const ConnectionCallback& cb)
	{
		connection_cb_ = cb;
	}
	void setMessageCallback(const MessageCallback& cb)
	{
		message_cb_ = cb;
	}
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{
		write_complete_cb_ = cb;
	}

	const std::string& ip_port() const { return ip_port_; }
	const std::string& name() const { return name_; }
	EventLoop* get_owner_loop() const { return owner_loop_; }

	void start();

private:
	void new_connection(SelectableFDPtr sockfd, const EndPoint& peeraddr);
	void remove_connection(const TcpConnectionPtr& conn);

	void remove_connection_in_loop(const TcpConnectionPtr& conn);

private:
	using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

	EventLoop* owner_loop_{nullptr};
	const std::string name_;
	const std::string ip_port_;

	// ATOMIC_FLAG_INIT is macros
	std::atomic_flag started_ = ATOMIC_FLAG_INIT;

	std::unique_ptr<Acceptor> acceptor_;

	// user callback function
	ConnectionCallback connection_cb_;
	MessageCallback message_cb_;
	WriteCompleteCallback write_complete_cb_;
	ThreadInitCallback thread_init_cb_;

	std::atomic<int> next_conn_id_{0};
	ConnectionMap connections_;

	DISALLOW_COPY_AND_ASSIGN(TcpServer);
};

}	// namespace annety

#endif	// ANT_TCP_SERVER_H_
