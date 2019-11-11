// By: wlmwang
// Date: Aug 03 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "synchronization/MutexLock.h"
#include "codec/LengthHeaderCodec.h"
#include "codec/StringEofCodec.h"

#include <iostream>	// std::cin
#include <string>	// std::getline
#include <stdio.h>

using namespace annety;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class ChatClient
{
public:
	// MSS = 136. MTU = 140
	ChatClient(EventLoop* loop, const EndPoint& addr)
		// : codec_(loop, LengthHeaderCodec::kLengthType32, 136)
		: codec_(loop, "\r\n", 136)
	{
		client_ = make_tcp_client(loop, addr, "ChatClient");

		client_->set_connect_callback(
			std::bind(&ChatClient::on_connect, this, _1));
		client_->set_message_callback(
			std::bind(&LengthHeaderCodec::decode_read, &codec_, _1, _2, _3));

		codec_.set_message_callback(
			std::bind(&ChatClient::on_message, this, _1, _2, _3));

		client_->enable_retry();
	}

	void connect()
	{
		client_->connect();
	}

	void disconnect()
	{
		client_->disconnect();
	}
	
	void stop()
	{
		client_->stop();
	}

	void write(const std::string& mesg)
	{
		NetBuffer buff;
		buff.append(mesg.data(), mesg.size());

		AutoLock locked(lock_);
		if (connection_) {
			codec_.endcode_send(connection_, &buff);
		}
	}

private:
	void on_connect(const TcpConnectionPtr& conn);

	void on_message(const TcpConnectionPtr& conn, NetBuffer* mesg, TimeStamp time);

private:
	TcpClientPtr client_;

	// LengthHeaderCodec codec_;
	StringEofCodec codec_;
	
	MutexLock lock_;
	TcpConnectionPtr connection_;
};

void ChatClient::on_connect(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChatClient - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	AutoLock locked(lock_);
	if (conn->connected()) {
		connection_ = conn;
	} else {
		connection_.reset();
	}
}

void ChatClient::on_message(const TcpConnectionPtr& conn, NetBuffer* mesg, TimeStamp time)
{
	::printf("<<< %s\n", mesg->to_string().c_str());
}

int main(int argc, char* argv[])
{
	set_min_log_severity(annety::LOG_DEBUG);

	EventLoopThread ioloop;
	ChatClient client(ioloop.start_loop(), EndPoint(1669));
	
	client.connect();
	
	std::string line;
	while (std::getline(std::cin, line)) {
		client.write(line);
	}
	client.disconnect();
	client.stop();
}
