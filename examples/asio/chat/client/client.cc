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
using CodecPtr = std::unique_ptr<Codec>;

class ChatClient
{
public:
	// MSS = 408. MTU = 512 + (enable_checksum? 4 : 0)
	ChatClient(EventLoop* loop, const EndPoint& addr)
		: codec_(new LengthHeaderCodec(loop, LengthHeaderCodec::kLengthType32, true, 408))
		// : codec_(new StringEofCodec(loop, "\r\n", 408))
	{
		client_ = make_tcp_client(loop, addr, "ChatClient");

		client_->set_connect_callback(
			std::bind(&ChatClient::on_connect, this, _1));
		client_->set_message_callback(
			std::bind(&LengthHeaderCodec::decode_read, codec_.get(), _1, _2, _3));

		codec_->set_message_callback(
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
			codec_->endcode_send(connection_, &buff);
		}
	}

private:
	void on_connect(const TcpConnectionPtr& conn);

	void on_message(const TcpConnectionPtr& conn, NetBuffer* mesg, TimeStamp);

private:
	TcpClientPtr client_;

	CodecPtr codec_;
	
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

void ChatClient::on_message(const TcpConnectionPtr& conn, NetBuffer* mesg, TimeStamp)
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
