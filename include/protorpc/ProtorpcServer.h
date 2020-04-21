// By: wlmwang
// Date: Oct 18 2019

#ifndef ANT_PROTORPC_PROTORPC_SERVER_H
#define ANT_PROTORPC_PROTORPC_SERVER_H

#include "TcpServer.h"
#include "EventLoop.h"
#include "Logging.h"
#include "protorpc/ProtorpcChannel.h"

#include <map>
#include <string>
#include <memory>

namespace annety
{
// Wrapper server with protobuf rpc
class ProtorpcServer
{
public:
	ProtorpcServer(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, chan_(new ProtorpcChannel(loop))
	{
		server_ = make_tcp_server(loop, addr, "ProtorpcServer");

		server_->set_connect_callback(
			std::bind(&ProtorpcServer::new_connection, this, _1));
	}

	void set_thread_num(int num_threads)
	{
		server_->set_thread_num(num_threads);
	}

	void start()
	{
		server_->start();
	}

	void listen(::google::protobuf::Service* service)
	{
		chan_->listen(service);
	}

private:
	void new_connection(const TcpConnectionPtr& conn);

private:
	EventLoop* loop_;
	TcpServerPtr server_;
	ProtorpcChannelPtr chan_;

	DISALLOW_COPY_AND_ASSIGN(ProtorpcServer);
};

void ProtorpcServer::new_connection(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "ProtorpcServer - " << conn->local_addr().to_ip_port() << " <- "
		<< conn->peer_addr().to_ip_port() << " s is "
		<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		chan_->attach_connection(conn);

		conn->set_message_callback(
			std::bind(&ProtorpcChannel::recv, chan_.get(), _1, _2, _3));
	} else {
		chan_->detach_connection();
	}
}

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_SERVER_H
