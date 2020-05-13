// By: wlmwang
// Date: Aug 04 2019

#include "client.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

DiscardClient::DiscardClient(annety::EventLoop* loop, const annety::EndPoint& addr, int size)
	: loop_(loop)
	, message_(size, 'A')
{
	client_ = make_tcp_client(loop, addr, "DiscardClient");

	client_->set_connect_callback(
		std::bind(&DiscardClient::on_connect, this, _1));
	client_->set_close_callback(
		std::bind(&DiscardClient::on_close, this, _1));
	client_->set_message_callback(
		std::bind(&DiscardClient::on_message, this, _1, _2, _3));
	client_->set_write_complete_callback(
		std::bind(&DiscardClient::on_write_complete, this, _1));

	client_->enable_retry();
}

void DiscardClient::connect()
{
	client_->connect();
}

void DiscardClient::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DiscardClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	conn->set_tcp_nodelay(true);
	conn->send(message_);
}

void DiscardClient::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DiscardClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";

	loop_->quit();
}

void DiscardClient::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp)

{
	buf->has_read_all();
}

void DiscardClient::on_write_complete(const annety::TcpConnectionPtr& conn)
{
	conn->send(message_);
}

