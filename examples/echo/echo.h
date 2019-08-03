// Refactoring: Anny Wang
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

	void start();

private:
	void on_connection(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::TcpServer server_;
};

#endif  // EXAMPLES_ECHO_ECHO_H
