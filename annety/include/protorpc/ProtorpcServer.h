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

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

namespace annety
{
// Wrapper server with protobuf rpc
class ProtorpcServer
{
public:
	ProtorpcServer(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
	{
		server_ = make_tcp_server(loop, addr, "ProtorpcServer");

		using std::placeholders::_1;
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

	void listen(::google::protobuf::Service* srv)
	{
		const google::protobuf::ServiceDescriptor* desc = srv->GetDescriptor();
		services_[desc->full_name()] = srv;
	}

private:
	void new_connection(const TcpConnectionPtr& conn);

private:
	EventLoop* loop_;
	TcpServerPtr server_;

	std::map<std::string, ::google::protobuf::Service*> services_;
};

void ProtorpcServer::new_connection(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "ProtorpcServer - " << conn->local_addr().to_ip_port() << " <- "
		<< conn->peer_addr().to_ip_port() << " s is "
		<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;

		ProtorpcChannelPtr chan(new ProtorpcChannel(loop_, conn));
		chan->set_services(&services_);
		conn->set_message_callback(
			std::bind(&ProtorpcChannel::recv, chan.get(), _1, _2, _3));
		
		// extend life cycle
		conn->set_context(chan);
	} else {
		// release life cycle
		conn->set_context(ProtorpcChannelPtr());
	}
}

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_SERVER_H
