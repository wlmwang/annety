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
// Server wrapper of protobuf rpc.
class ProtorpcServer
{
public:
	ProtorpcServer(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, channel_(new ProtorpcChannel(loop))
	{
		CHECK(loop);

		server_ = make_tcp_server(loop, addr, "ProtorpcServer");
		
		using std::placeholders::_1;

		server_->set_connect_callback(
			std::bind(&ProtorpcServer::new_connection, this, _1));
		server_->set_close_callback(
			std::bind(&ProtorpcServer::remove_connection, this, _1));
	}

	void set_thread_num(int num_threads)
	{
		server_->set_thread_num(num_threads);
	}

	void start()
	{
		server_->start();
	}

	// Add service impl instance.
	void listen(::google::protobuf::Service* service)
	{
		channel_->listen(service);
	}

private:
	void new_connection(const TcpConnectionPtr& conn);
	void remove_connection(const TcpConnectionPtr&);

private:
	EventLoop* loop_;
	TcpServerPtr server_;
	ProtorpcChannelPtr channel_;

	DISALLOW_COPY_AND_ASSIGN(ProtorpcServer);
};

void ProtorpcServer::new_connection(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "ProtorpcServer - " << conn->local_addr().to_ip_port() << " <- "
		<< conn->peer_addr().to_ip_port() << " s is "
		<< "UP";
	
	channel_->attach_connection(conn);
}

void ProtorpcServer::remove_connection(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "ProtorpcServer - " << conn->local_addr().to_ip_port() << " <- "
		<< conn->peer_addr().to_ip_port() << " s is "
		<< "DOWN";

	channel_->detach_connection();
}

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_SERVER_H
