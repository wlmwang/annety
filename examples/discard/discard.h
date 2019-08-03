// Refactoring: Anny Wang
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
	void on_connection(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::Time time);

private:
	annety::TcpServer server_;
};

#endif  // EXAMPLES_DISCARD_DISCARD_H
