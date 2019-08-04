// Refactoring: Anny Wang
// Date: Aug 04 2019

#ifndef EXAMPLES_ECHO_ECHO_H
#define EXAMPLES_ECHO_ECHO_H

#include "TcpClient.h"
#include "EventLoop.h"

// RFC 864
class ChargenClient
{
public:
	ChargenClient(annety::EventLoop* loop, const annety::EndPoint& addr);

	void connect();

private:
	void on_connect(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::EventLoop* loop_;
	annety::TcpClient client_;
};

#endif  // EXAMPLES_ECHO_ECHO_H
