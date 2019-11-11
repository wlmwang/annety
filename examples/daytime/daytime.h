// By: wlmwang
// Date: Aug 03 2019

#ifndef EXAMPLES_DAYTIME_DAYTIME_H
#define EXAMPLES_DAYTIME_DAYTIME_H

#include "TcpServer.h"
#include "EventLoop.h"

// RFC 867
class DaytimeServer
{
public:
	DaytimeServer(annety::EventLoop* loop, const annety::EndPoint& addr);

	void start();

private:
	void on_connection(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::TcpServerPtr server_;
};

#endif  // EXAMPLES_DAYTIME_DAYTIME_H
