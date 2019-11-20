// By: wlmwang
// Date: Oct 18 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "synchronization/MutexLock.h"
#include "protorpc/ProtorpcChannel.h"

#include "Query.pb.h"

#include <iostream>	// std::cin
#include <string>	// std::getline
#include <stdio.h>

using namespace annety;
using namespace examples::protobuf::rpc;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class QueryClient
{
public:
	QueryClient(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, chan_(new ProtorpcChannel(loop))
		, stub_(chan_.get())
	{
		client_ = make_tcp_client(loop, addr, "QueryClient");

		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;
		client_->set_connect_callback(
			std::bind(&QueryClient::on_connect, this, _1));
		client_->set_message_callback(
			std::bind(&ProtorpcChannel::recv, chan_.get(), _1, _2, _3));
	}

	void connect()
	{
		client_->connect();
	}

private:
	void on_connect(const TcpConnectionPtr&);
	
	void result(QueryResponse* resp)
	{
		LOG(INFO) << "QueryClient:result - " << resp->DebugString();
		// finish rpc
		client_->disconnect();
	}

private:
	EventLoop* loop_;
	TcpClientPtr client_;
	MutexLock lock_;

	ProtorpcChannelPtr chan_;
	QueryService::Stub stub_;
};

void QueryClient::on_connect(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "QueryClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		chan_->set_connection(conn);
		
		QueryResponse* res = new QueryResponse;
		QueryRequest req;
		req.set_checkerboard("001010");

		stub_.Solve(nullptr, &req, res, NewCallback(this, &QueryClient::result, res));
	} else {
		loop_->quit();
	}
}

int main(int argc, char* argv[])
{
	set_min_log_severity(annety::LOG_DEBUG);
	
	EventLoop loop;
	QueryClient client(&loop, EndPoint(1669));
	
	client.connect();
	loop.loop();

	google::protobuf::ShutdownProtobufLibrary();
}
