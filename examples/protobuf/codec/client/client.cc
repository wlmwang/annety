// By: wlmwang
// Date: Oct 12 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "synchronization/MutexLock.h"
#include "protobuf/ProtobufCodec.h"
#include "protobuf/ProtobufDispatch.h"

#include "Query.pb.h"

#include <iostream>	// std::cin
#include <string>	// std::getline
#include <stdio.h>

using namespace annety;
using namespace examples::protobuf::codec;

using EmptyPtr = std::shared_ptr<Empty>;
using AnswerPtr = std::shared_ptr<Answer>;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class QueryClient
{
public:
	QueryClient(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, codec_(loop, std::bind(&ProtobufDispatch::dispatch, &dispatch_, _1, _2, _3))
	{
		client_ = make_tcp_client(loop, addr, "QueryClient");

		client_->set_connect_callback(
			std::bind(&QueryClient::on_connect, this, _1));
		client_->set_close_callback(
			std::bind(&QueryClient::on_close, this, _1));
		client_->set_message_callback(
			std::bind(&ProtobufCodec::recv, &codec_, _1, _2, _3));

		// Register protobuf message callbacks.
		dispatch_.add<Answer>(
			std::bind(&QueryClient::on_answer, this, _1, _2, _3));
		dispatch_.add<Empty>(
			std::bind(&QueryClient::on_empty, this, _1, _2, _3));

		client_->enable_retry();
	}

	void connect()
	{
		client_->connect();
	}

private:
	void on_connect(const TcpConnectionPtr&);
	void on_close(const TcpConnectionPtr&);

	void on_empty(const TcpConnectionPtr&, const EmptyPtr&, TimeStamp);
	void on_answer(const TcpConnectionPtr&, const AnswerPtr&, TimeStamp);

private:
	EventLoop* loop_;
	TcpClientPtr client_;
	MutexLock lock_;

	ProtobufDispatch dispatch_;
	ProtobufCodec codec_;
};

void QueryClient::on_connect(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "QueryClient - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	Query query;
	query.set_id(1);
	query.set_questioner("Anny Wang");
	query.add_question("Hello???");
		
	codec_.send(conn, query);
}

void QueryClient::on_close(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "QueryClient - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";

	loop_->quit();
}

void QueryClient::on_answer(const TcpConnectionPtr& conn, const AnswerPtr& mesg, TimeStamp)
{
	LOG(INFO) << "QueryClient::on_answer";
	LOG(INFO) << mesg->GetTypeName() << mesg->DebugString();
}

void QueryClient::on_empty(const TcpConnectionPtr& conn, const EmptyPtr& mesg, TimeStamp)
{
	LOG(INFO) << "QueryClient::on_empty";
	LOG(INFO) << mesg->GetTypeName() << mesg->DebugString();
}

int main(int argc, char* argv[])
{	
	EventLoop loop;

	QueryClient client(&loop, EndPoint(1669));	
	client.connect();

	loop.loop();

	google::protobuf::ShutdownProtobufLibrary();
}
