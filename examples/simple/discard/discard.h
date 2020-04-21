// By: wlmwang
// Date: Aug 03 2019

#ifndef EXAMPLES_DISCARD_DISCARD_H
#define EXAMPLES_DISCARD_DISCARD_H

#include "TcpServer.h"
#include "EventLoop.h"

// RFC 863
class DiscardServer
{
public:
	DiscardServer(annety::EventLoop* loop, const annety::EndPoint& addr);

	void start();

private:
	void on_connect(const annety::TcpConnectionPtr& conn);
	void on_close(const annety::TcpConnectionPtr& conn);
	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::TcpServerPtr server_;
};

#endif  // EXAMPLES_DISCARD_DISCARD_H
