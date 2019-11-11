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
	server_->set_message_callback(
		std::bind(&EchoServer::on_message, this, _1, _2, _3));

	loop->run_every(5.0, std::bind(&EchoServer::print_throughput, this));
}

void EchoServer::start()
{
	server_->start();
}

void EchoServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "EchoServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");
	
	if (conn->connected()) {
		conn->set_tcp_nodelay(true);
	}
}

void EchoServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	received_messages_++;
	transferred_ += buf->readable_bytes();
	
	conn->send(buf);
}

void EchoServer::print_throughput()
{
	annety::TimeStamp curr = annety::TimeStamp::now();
	int64_t newcounter = transferred_;
	int64_t bytes = newcounter - counter_;
	int64_t msgs = received_messages_.fetch_and(0);
	double time = (curr - start_time_).in_seconds_f();

	LOG(WARNING) << static_cast<double>(bytes)/(time*1024*1024) << " MiB/s "
		<< static_cast<double>(msgs)/(time*1024) << " Ki Msgs/s "
		<< static_cast<double>(bytes)/msgs << " bytes per msg";

	counter_ = newcounter;
	start_time_ = curr;
}
