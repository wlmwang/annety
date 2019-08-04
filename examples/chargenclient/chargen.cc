// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "chargen.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

ChargenClient::ChargenClient(annety::EventLoop* loop, const annety::EndPoint& addr)
	: loop_(loop)
	, client_(loop, addr, "ChargenClient")
{
	client_.set_connect_callback(
		std::bind(&ChargenClient::on_connect, this, _1));
	client_.set_message_callback(
		std::bind(&ChargenClient::on_message, this, _1, _2, _3));
	// client_.enable_retry();

	// 屏蔽日志
	annety::set_min_log_severity(annety::LOG_INFO);
}

void ChargenClient::connect()
{
	client_.connect();
}

void ChargenClient::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChargenClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (!conn->connected()) {
    	loop_->quit();
	}
}

void ChargenClient::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	buf->has_read_all();
	// std::string message(buf->taken_as_string());
	// LOG(INFO) << conn->name() << " echo " << (int)message.size() << " bytes, "
	// 	<< "data received at " << time;
}
