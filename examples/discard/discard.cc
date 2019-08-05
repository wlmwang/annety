// Refactoring: Anny Wang
// Date: Aug 03 2019

#include "discard.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

DiscardServer::DiscardServer(annety::EventLoop* loop, const annety::EndPoint& addr)
	: server_(loop, addr, "DiscardServer")
{
	server_.set_connect_callback(
		std::bind(&DiscardServer::on_connection, this, _1));
	server_.set_message_callback(
		std::bind(&DiscardServer::on_message, this, _1, _2, _3));
}

void DiscardServer::start()
{
	server_.start();
}

void DiscardServer::on_connection(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "DiscardServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");
}

void DiscardServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	std::string message(buf->taken_as_string());
	
	LOG(INFO) << conn->name() << " discards " << (int)message.size() << " bytes, "
		<< "data received at " << time;
}