// By: wlmwang
// Date: Aug 03 2019

#include "echo.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(annety::EventLoop* loop, const annety::EndPoint& addr)
{
	server_ = make_tcp_server(loop, addr, "EchoServer");

	server_->set_connect_callback(
		std::bind(&EchoServer::on_connect, this, _1));
	server_->set_close_callback(
		std::bind(&EchoServer::on_close, this, _1));
	server_->set_message_callback(
		std::bind(&EchoServer::on_message, this, _1, _2, _3));
}

void EchoServer::listen()
{
	server_->listen();
}

void EchoServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "EchoServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";
}

void EchoServer::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "EchoServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";
}

void EchoServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp receive)

{
	std::string message(buf->taken_as_string());
	
	LOG(INFO) << conn->name() << " echo " << static_cast<int>(message.size()) << " bytes, "
		<< "data received at " << receive;

	conn->send(message);
}
