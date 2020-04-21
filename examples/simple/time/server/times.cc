// By: wlmwang
// Date: Aug 03 2019

#include "times.h"
#include "Logging.h"
#include "ByteOrder.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

TimeServer::TimeServer(annety::EventLoop* loop, const annety::EndPoint& addr)
{
	server_ = make_tcp_server(loop, addr, "TimeServer");

	server_->set_connect_callback(
		std::bind(&TimeServer::on_connect, this, _1));
	server_->set_close_callback(
		std::bind(&TimeServer::on_close, this, _1));
	server_->set_message_callback(
		std::bind(&TimeServer::on_message, this, _1, _2, _3));
}

void TimeServer::start()
{
	server_->start();
}

void TimeServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "TimeServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	annety::TimeStamp curr = annety::TimeStamp::now();
	int32_t be32 = annety::host_to_net32(static_cast<int32_t>(curr.to_time_t()));
	conn->send(&be32, sizeof be32);
	conn->shutdown();
}

void TimeServer::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "TimeServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";
}

void TimeServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	std::string message(buf->taken_as_string());
	
	LOG(INFO) << conn->name() << " times " << static_cast<int>(message.size()) << " bytes, "
		<< "data received at " << time;
}
