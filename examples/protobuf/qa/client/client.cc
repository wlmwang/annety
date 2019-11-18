// By: wlmwang
// Date: Oct 12 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "synchronization/MutexLock.h"
#include "codec/ProtobufCodec.h"
#include "codec/ProtobufDispatcher.h"

#include "Query.pb.h"

#include <iostream>	// std::cin
#include <string>	// std::getline
#include <stdio.h>

using namespace annety;
using namespace qa;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using EmptyPtr = std::shared_ptr<qa::Empty>;
using AnswerPtr = std::shared_ptr<qa::Answer>;

class QueryClient
{
public:
	QueryClient(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, codec_(loop)
	{
		client_ = make_tcp_client(loop, addr, "QueryClient");

		client_->set_connect_callback(
			std::bind(&QueryClient::on_connect, this, _1));
		client_->set_message_callback(
			std::bind(&ProtobufCodec::recv, &codec_, _1, _2, _3));

		codec_.set_dispatch_callback(
			std::bind(&ProtobufDispatcher::dispatch, &dispatcher_, _1, _2, _3));

		dispatcher_.register_message_cb<qa::Answer>(
			std::bind(&QueryClient::on_answer, this, _1, _2, _3));
		dispatcher_.register_message_cb<qa::Empty>(
			std::bind(&QueryClient::on_empty, this, _1, _2, _3));

		client_->enable_retry();
	}

	void connect()
	{
		client_->connect();
	}

private:
	void on_connect(const TcpConnectionPtr&);

	void on_unknown_message(const TcpConnectionPtr&, const MessagePtr&, TimeStamp);
	void on_empty(const TcpConnectionPtr&, const EmptyPtr&, TimeStamp);
	void on_answer(const TcpConnectionPtr&, const AnswerPtr&, TimeStamp);

private:
	EventLoop* loop_;
	TcpClientPtr client_;
	MutexLock lock_;

	ProtobufDispatcher dispatcher_;
	ProtobufCodec codec_;
};

void QueryClient::on_connect(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "QueryClient - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		Query query;
		query.set_id(1);
		query.set_questioner("Anny Wang");
		query.add_question("Hello?");
		
		codec_.send(conn, query);
	}
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
	set_min_log_severity(annety::LOG_DEBUG);
	
	EventLoop loop;
	QueryClient client(&loop, EndPoint(1669));
	
	client.connect();
	loop.loop();
}
