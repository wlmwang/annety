// By: wlmwang
// Date: Aug 04 2019

#ifndef EXAMPLES_TIME_CLIENT_TIMES_H
#define EXAMPLES_TIME_CLIENT_TIMES_H

#include "TcpClient.h"
#include "EventLoop.h"

// RFC 868
class TimeClient
{
public:
	TimeClient(annety::EventLoop* loop, const annety::EndPoint& addr);

	~TimeClient();

	void connect();

private:
	void on_connect(const annety::TcpConnectionPtr& conn);
	void on_close(const annety::TcpConnectionPtr& conn);
	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

private:
	annety::EventLoop* loop_;
	annety::TcpClientPtr client_;
};

#endif  // EXAMPLES_TIME_CLIENT_TIMES_H
