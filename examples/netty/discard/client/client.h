// By: wlmwang
// Date: Aug 04 2019

#ifndef EXAMPLES_NETTY_DISCARD_CLIENT_DISCARD_H
#define EXAMPLES_NETTY_DISCARD_CLIENT_DISCARD_H

#include "TcpClient.h"
#include "EventLoop.h"

class DiscardClient
{
public:
	DiscardClient(annety::EventLoop* loop, const annety::EndPoint& addr, int size = 512);

	void connect();

private:
	void on_connect(const annety::TcpConnectionPtr& conn);
	void on_close(const annety::TcpConnectionPtr& conn);
	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);
	void on_write_complete(const annety::TcpConnectionPtr& conn);
	
private:
	annety::EventLoop* loop_;
	annety::TcpClientPtr client_;

	std::string message_;
};

#endif  // EXAMPLES_NETTY_DISCARD_CLIENT_DISCARD_H
