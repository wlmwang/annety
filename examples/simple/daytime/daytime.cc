// By: wlmwang
// Date: Aug 03 2019

#include "daytime.h"
#include "TimeStamp.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

DaytimeServer::DaytimeServer(annety::EventLoop* loop, const annety::EndPoint& addr)
{
	server_ = make_tcp_server(loop, addr, "DaytimeServer");

	server_->set_connect_callback(
		std::bind(&DaytimeServer::on_connect, this, _1));
	server_->set_close_callback(
		std::bind(&DaytimeServer::on_close, this, _1));
	server_->set_message_callback(
		std::bind(&DaytimeServer::on_message, this, _1, _2, _3));
}

void DaytimeServer::start()
{
	server_->start();
}

void DaytimeServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DaytimeServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	annety::TimeStamp curr = annety::TimeStamp::now();
	annety::TimeStamp::Exploded exploded;
	curr.to_utc_explode(&exploded);

	conn->send(exploded.to_formatted_string() + "\n");
	conn->shutdown();
}

void DaytimeServer::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DaytimeServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";
}

void DaytimeServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	std::string message(buf->taken_as_string());
	
	LOG(INFO) << conn->name() << " daytime " << static_cast<int>(message.size()) << " bytes, "
		<< "data received at " << time;
}
