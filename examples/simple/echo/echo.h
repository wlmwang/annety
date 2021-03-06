// By: wlmwang
// Date: Aug 03 2019

#ifndef EXAMPLES_ECHO_ECHO_H
#define EXAMPLES_ECHO_ECHO_H

#include "TcpServer.h"
#include "EventLoop.h"

// RFC 862
class EchoServer
{
public:
	EchoServer(annety::EventLoop* loop, const annety::EndPoint& addr);

	void listen();

private:
	void on_connect(const annety::TcpConnectionPtr& conn);
	void on_close(const annety::TcpConnectionPtr& conn);
	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::TcpServerPtr server_;
};

#endif  // EXAMPLES_ECHO_ECHO_H
