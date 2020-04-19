// By: wlmwang
// Date: Aug 04 2019

#ifndef EXAMPLES_CHARGEN_SERVER_CHARGEN_H
#define EXAMPLES_CHARGEN_SERVER_CHARGEN_H

#include "TcpServer.h"
#include "EventLoop.h"
#include "TimeStamp.h"

#include <atomic>
#include <string>

// RFC 864
class ChargenServer
{
public:
	ChargenServer(annety::EventLoop* loop, const annety::EndPoint& addr, bool print = true);

	void start();

private:
	void on_connection(const annety::TcpConnectionPtr& conn);

	void on_message(const annety::TcpConnectionPtr& conn,
			annety::NetBuffer* buf, annety::TimeStamp time);

	void on_write_complete(const annety::TcpConnectionPtr& conn);
	void print_throughput();

private:
	annety::TcpServerPtr server_;

	std::string message_;
	std::atomic<int64_t> transferred_{0};
	annety::TimeStamp start_time_{annety::TimeStamp::now()};
};

#endif  // EXAMPLES_CHARGEN_SERVER_CHARGEN_H
