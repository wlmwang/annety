// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "times.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

TimeClient::TimeClient(annety::EventLoop* loop, const annety::EndPoint& addr)
	: loop_(loop)
{
	client_ = make_tcp_client(loop, addr, "TimeClient");

	client_->set_connect_callback(
		std::bind(&TimeClient::on_connect, this, _1));
	client_->set_error_callback(
			std::bind(&TimeClient::on_error, this));
	client_->set_message_callback(
		std::bind(&TimeClient::on_message, this, _1, _2, _3));
	// client_->enable_retry();
}

void TimeClient::connect()
{
	client_->connect();
}

void TimeClient::on_error()
{
	client_->stop();
	loop_->quit();
}

void TimeClient::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "TimeClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (!conn->connected()) {
    	loop_->quit();
	}
}

void TimeClient::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	if (buf->readable_bytes() >= sizeof(int32_t)) {
		annety::TimeStamp ts = annety::TimeStamp::from_time_t(buf->read_int32());
		LOG(INFO) << "Server time = " << time << ", " << ts;
	} else {
		LOG(INFO) << conn->name() << " no enough data " << buf->readable_bytes()
			<< " at " << time;
	}
}
