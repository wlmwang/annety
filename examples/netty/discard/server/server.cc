// By: wlmwang
// Date: Aug 03 2019

#include "server.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

DiscardServer::DiscardServer(annety::EventLoop* loop, const annety::EndPoint& addr)
{
	server_ = make_tcp_server(loop, addr, "DiscardServer");

	server_->set_connect_callback(
		std::bind(&DiscardServer::on_connect, this, _1));
	server_->set_close_callback(
		std::bind(&DiscardServer::on_close, this, _1));
	server_->set_message_callback(
		std::bind(&DiscardServer::on_message, this, _1, _2, _3));

	loop->run_every(5.0, std::bind(&DiscardServer::print_throughput, this));
}

void DiscardServer::listen()
{
	server_->listen();
}

void DiscardServer::set_thread_num(int num)
{
	server_->set_thread_num(num);
}

void DiscardServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DiscardServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";
	
	conn->set_tcp_nodelay(true);
}

void DiscardServer::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DiscardServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";
}

void DiscardServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp)

{
	received_messages_++;
	transferred_ += buf->readable_bytes();
	
	buf->has_read_all();
}

void DiscardServer::print_throughput()
{
	annety::TimeStamp curr = annety::TimeStamp::now();

	int64_t newcounter = transferred_;
	int64_t bytes = newcounter - counter_;
	int64_t msgs = received_messages_.exchange(0);
	double delta = (curr - start_time_).in_seconds_f();

	LOG(INFO) << static_cast<double>(bytes)/(delta*1024*1024) << " MiB/s "
		<< static_cast<double>(msgs)/(delta*1024) << " Ki Msgs/s "
		<< static_cast<double>(bytes)/msgs << " bytes per msg";

	counter_ = newcounter;
	start_time_ = curr;
}
