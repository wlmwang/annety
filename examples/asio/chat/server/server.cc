// Refactoring: Anny Wang
// Date: Aug 03 2019

#include "TcpServer.h"
#include "EventLoop.h"
#include "../LengthHeaderCodec.h"

#include <set>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using ConnectionList = std::set<annety::TcpConnectionPtr>;

class ChatServer
{
public:
	ChatServer(annety::EventLoop* loop, const annety::EndPoint& addr)
		: server_(loop, addr, "ChatServer")
		, codec_(std::bind(&ChatServer::on_packet_message, this, _1, _2, _3))
	{
		server_.set_connect_callback(
			std::bind(&ChatServer::on_connect, this, _1));
		server_.set_message_callback(
			std::bind(&LengthHeaderCodec::on_message, &codec_, _1, _2, _3));
	}

	void start()
	{
		server_.start();
	}

private:
	void on_connect(const annety::TcpConnectionPtr& conn);

	void on_packet_message(const annety::TcpConnectionPtr& conn,
			const std::string& message, annety::TimeStamp time);

private:
	annety::TcpServer server_;
	LengthHeaderCodec codec_;

	ConnectionList connections_;
};

void ChatServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChatServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		connections_.insert(conn);
	} else {
		connections_.erase(conn);
	}
}

void ChatServer::on_packet_message(const annety::TcpConnectionPtr& conn,
		const std::string& message, annety::TimeStamp time)

{
	ConnectionList::iterator it = connections_.begin();
	for (; it != connections_.end(); ++it) {
		codec_.send(*it, message);
	}
}

int main(int argc, char* argv[])
{
	annety::set_min_log_severity(annety::LOG_INFO);
	
	annety::EventLoop loop;
	ChatServer server(&loop, annety::EndPoint(1669));

	server.start();
	loop.loop();
}
