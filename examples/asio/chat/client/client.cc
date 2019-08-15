// Refactoring: Anny Wang
// Date: Aug 03 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "synchronization/MutexLock.h"
#include "../LengthHeaderCodec.h"

#include <iostream>	// std::cin
#include <string>	// std::getline
#include <stdio.h>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class ChatClient
{
public:
	ChatClient(annety::EventLoop* loop, const annety::EndPoint& addr)
		: codec_(std::bind(&ChatClient::on_packet_message, this, _1, _2, _3))
	{
		client_ = make_tcp_client(loop, addr, "ChatClient");

		client_->set_connect_callback(
			std::bind(&ChatClient::on_connect, this, _1));
		client_->set_message_callback(
			std::bind(&LengthHeaderCodec::on_message, &codec_, _1, _2, _3));
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

	void write(const std::string& message)
	{
		annety::AutoLock locked(lock_);
		if (connection_) {
			codec_.send(connection_, message);
		}
	}

private:
	void on_connect(const annety::TcpConnectionPtr& conn);

	void on_packet_message(const annety::TcpConnectionPtr& conn,
		const std::string& message, annety::TimeStamp time);

private:
	annety::TcpClientPtr client_;
	LengthHeaderCodec codec_;
	
	annety::MutexLock lock_;
	annety::TcpConnectionPtr connection_;
};

void ChatClient::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChatClient - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	annety::AutoLock locked(lock_);
	if (conn->connected()) {
		connection_ = conn;
	} else {
		connection_.reset();
	}
}

void ChatClient::on_packet_message(const annety::TcpConnectionPtr& conn,
		const std::string& message, annety::TimeStamp time)

{
	::printf("<<< %s\n", message.c_str());
}

int main(int argc, char* argv[])
{
	annety::set_min_log_severity(annety::LOG_DEBUG);

	annety::EventLoopThread ioloop;

	ChatClient client(ioloop.start_loop(), annety::EndPoint(1669));
	client.connect();
	
	std::string line;
	while (std::getline(std::cin, line)) {
		client.write(line);
	}
	client.disconnect();
	client.stop();
}
