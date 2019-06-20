// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TCP_SERVER_H_
#define ANT_TCP_SERVER_H_

#include "Macros.h"

#include <string>
#include <memory>

namespace annety
{
class EndPoint;
class Acceptor;
class EventLoop;
//class EventLoopThreadPool;

// TCP server, supports single-threaded and thread-pool models.
class TcpServer
{
public:
	//using ThreadInitCallback = std::function<void(EventLoop*)>;
	enum Option
	{
		kNoReusePort,
		kReusePort,
	};

	TcpServer(EventLoop* loop,
			  const EndPoint& listenAddr,
			  const std::string& nameArg,
			  Option option = kNoReusePort);

	~TcpServer();

	const std::string& ip_port() const { return ip_port_; }
	const std::string& name() const { return name_; }
	EventLoop* get_owner_loop() const { return owner_loop_; }

	void start();

	// // Set connection callback.
	// // Not thread safe.
	// void setConnectionCallback(const ConnectionCallback& cb)
	// {
	// 	connection_cb_ = cb;
	// }
	// // Set message callback.
	// // Not thread safe.
	// void setMessageCallback(const MessageCallback& cb)
	// {
	// 	message_cb_ = cb;
	// }
	// // Set write complete callback.
	// // Not thread safe.
	// void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	// {
	// 	write_complete_cb_ = cb;
	// }

private:
	// // Thread safe.
	// void removeConnection(const TcpConnectionPtr& conn);
	// // Not thread safe, but in loop
	// void removeConnectionInLoop(const TcpConnectionPtr& conn);
	// // Not thread safe, but in loop
	// void newConnection(int sockfd, const EndPoint& peerAddr);

private:
	// using ConnectionMap = std::map<string, TcpConnectionPtr>;

	EventLoop* owner_loop_{nullptr};
	const std::string ip_port_;
	const std::string name_;

	int started_{0};
	std::unique_ptr<Acceptor> acceptor_;

	//std::shared_ptr<EventLoopThreadPool> threadPool_;
	//ThreadInitCallback threadInitCallback_;

	// // user callback
	// ConnectionCallback connection_cb_;
	// MessageCallback message_cb_;
	// WriteCompleteCallback write_complete_cb_;
	
	// AtomicInt32 started_;
	// int nextConnId_;

	// ConnectionMap connections_;
};

}	// namespace annety

#endif	// ANT_TCP_SERVER_H_
