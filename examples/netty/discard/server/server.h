// By: wlmwang
// Date: Aug 03 2019

#ifndef EXAMPLES_NETTY_DISCARD_SERVER_DISCARD_H
#define EXAMPLES_NETTY_DISCARD_SERVER_DISCARD_H

#include "TcpServer.h"
#include "EventLoop.h"

#include <atomic>

class DiscardServer
{
public:
	DiscardServer(annety::EventLoop* loop, const annety::EndPoint& addr);

	void listen();
	
	void set_thread_num(int num);

private:
	void on_connect(const annety::TcpConnectionPtr& conn);
	void on_close(const annety::TcpConnectionPtr& conn);
	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

	void print_throughput();

private:
	annety::TcpServerPtr server_;

	std::atomic<int64_t> received_messages_{0};
	std::atomic<int64_t> transferred_{0};
	std::atomic<int64_t> counter_{0};
	annety::TimeStamp start_time_{annety::TimeStamp::now()};
};

#endif  // EXAMPLES_NETTY_DISCARD_SERVER_DISCARD_H
