// By: wlmwang
// Date: Aug 03 2019

#include "TcpServer.h"
#include "EventLoop.h"
#include "codec/LengthHeaderCodec.h"
#include "codec/StringEofCodec.h"

#include <iostream>
#include <memory>
#include <string>
#include <set>

using namespace annety;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using CodecPtr = std::unique_ptr<Codec>;
using ConnectionList = std::set<annety::TcpConnectionPtr>;

class ChatServer
{
public:
	// MSS = 408. MTU = 512 + (enable_checksum? 4 : 0)
	ChatServer(EventLoop* loop, const EndPoint& addr)
		: codec_(new LengthHeaderCodec(loop, LengthHeaderCodec::kLengthType32, true, 408))
		// : codec_(new StringEofCodec(loop, "\r\n", 408))
	{
		server_ = make_tcp_server(loop, addr, "ChatServer");

		server_->set_connect_callback(
			std::bind(&ChatServer::on_connect, this, _1));
		server_->set_close_callback(
			std::bind(&ChatServer::on_close, this, _1));
		server_->set_message_callback(
			std::bind(&Codec::recv, codec_.get(), _1, _2, _3));
		
		codec_->set_message_callback(
			std::bind(&ChatServer::on_message, this, _1, _2, _3));
	}

	void start()
	{
		server_->start();
	}

private:
	void on_connect(const TcpConnectionPtr& conn);
	void on_close(const TcpConnectionPtr& conn);
	void on_message(const TcpConnectionPtr& conn, NetBuffer* mesg, TimeStamp);

private:
	TcpServerPtr server_;
	CodecPtr codec_;

	ConnectionList connections_;
};

void ChatServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChatServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	connections_.insert(conn);
}

void ChatServer::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChatServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";

	connections_.erase(conn);
}

void ChatServer::on_message(const TcpConnectionPtr& conn, NetBuffer* mesg, TimeStamp)
{
	// broadcast
	ConnectionList::iterator it = connections_.begin();
	for (; it != connections_.end(); ++it) {
		codec_->send(*it, mesg);
	}
}

int main(int argc, char* argv[])
{
	set_min_log_severity(LOG_DEBUG);
	
	EventLoop loop;
	
	ChatServer server(&loop, EndPoint(1669));
	server.start();

	loop.loop();
}
