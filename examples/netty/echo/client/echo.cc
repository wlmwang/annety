// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "echo.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoClient::EchoClient(annety::EventLoop* loop, const annety::EndPoint& addr, int size)
	: loop_(loop)
	, client_(loop, addr, "EchoClient")
	, message_(size, 'A')
{
	client_.set_connect_callback(
		std::bind(&EchoClient::on_connect, this, _1));
	client_.set_message_callback(
		std::bind(&EchoClient::on_message, this, _1, _2, _3));
	// client_.enable_retry();
}

void EchoClient::connect()
{
	client_.connect();
}

void EchoClient::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "EchoClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		conn->set_tcp_nodelay(true);
		conn->send(message_);
	} else {
		loop_->quit();
	}
}

void EchoClient::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	// echo back
	conn->send(buf);
}
