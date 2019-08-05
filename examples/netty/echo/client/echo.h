// Refactoring: Anny Wang
// Date: Aug 04 2019

#ifndef EXAMPLES_NETTY_ECHO_CLIENT_ECHO_H
#define EXAMPLES_NETTY_ECHO_CLIENT_ECHO_H

#include "TcpClient.h"
#include "EventLoop.h"

class EchoClient
{
public:
	EchoClient(annety::EventLoop* loop, const annety::EndPoint& addr, int size = 512);

	void connect();

private:
	void on_connect(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::EventLoop* loop_;
	annety::TcpClient client_;

	std::string message_;
};

#endif  // EXAMPLES_NETTY_ECHO_CLIENT_ECHO_H
