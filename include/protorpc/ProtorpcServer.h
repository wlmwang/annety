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

		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;

		server_ = make_tcp_server(loop, addr, "ProtorpcServer");
		
		server_->set_connect_callback(
			std::bind(&ProtorpcServer::new_connection, this, _1));
		server_->set_close_callback(
			std::bind(&ProtorpcServer::remove_connection, this, _1));
		server_->set_message_callback(
			std::bind(&ProtorpcChannel::recv, channel_.get(), _1, _2, _3));
	}

	// Starts the server if it's not listenning.
	void listen()
	{
		server_->listen();
	}

	// Turn on the server to become a multi-threaded model.
	// *Not thread safe*, but usually be called before listen().
	void set_thread_num(int num_threads)
	{
		server_->set_thread_num(num_threads);
	}

	// Add service impl instance.
	// *Not thread safe*, but usually be called before listen().
	void add(::google::protobuf::Service* service)
	{
		channel_->add(service);
	}

private:
	void new_connection(const TcpConnectionPtr&);
	void remove_connection(const TcpConnectionPtr&);

private:
	EventLoop* loop_;
	TcpServerPtr server_;
	ProtorpcChannelPtr channel_;

	DISALLOW_COPY_AND_ASSIGN(ProtorpcServer);
};

void ProtorpcServer::new_connection(const TcpConnectionPtr& conn)
{
	loop_->check_in_own_loop();

	DLOG(TRACE) << "ProtorpcServer - " << conn->local_addr().to_ip_port() << " <- "
		<< conn->peer_addr().to_ip_port() << " s is "
		<< "UP";
	
	channel_->attach_connection(conn);
}

void ProtorpcServer::remove_connection(const TcpConnectionPtr& conn)
{
	loop_->check_in_own_loop();

	DLOG(TRACE) << "ProtorpcServer - " << conn->local_addr().to_ip_port() << " <- "
		<< conn->peer_addr().to_ip_port() << " s is "
		<< "DOWN";

	channel_->detach_connection();
}

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_SERVER_H
