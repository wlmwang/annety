// By: wlmwang
// Date: Aug 03 2019

#ifndef EXAMPLES_NETTY_ECHO_SERVER_ECHO_H
#define EXAMPLES_NETTY_ECHO_SERVER_ECHO_H

#include "TcpServer.h"
#include "EventLoop.h"

#include <atomic>

class EchoServer
{
public:
	EchoServer(annety::EventLoop* loop, const annety::EndPoint& addr);

	void start();
	
	// for main() to set_thread_num 
	annety::TcpServerPtr server() 
	{
		return server_;
	}

private:
	void on_connect(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

	void print_throughput();

private:
	annety::TcpServerPtr server_;

	annety::TimeStamp start_time_{annety::TimeStamp::now()};
	std::atomic<int64_t> received_messages_{0};
	std::atomic<int64_t> transferred_{0};
	std::atomic<int64_t> counter_{0};
};

#endif  // EXAMPLES_NETTY_ECHO_SERVER_ECHO_H
