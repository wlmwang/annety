// By: wlmwang
// Date: Oct 12 2019

#include "TcpServer.h"
#include "EventLoop.h"
#include "codec/ProtobufCodec.h"
#include "codec/ProtobufDispatcher.h"

#include "Query.pb.h"

#include <iostream>
#include <memory>
#include <string>
#include <set>

using namespace annety;
using namespace qa;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using QueryPtr = std::shared_ptr<qa::Query>;
using AnswerPtr = std::shared_ptr<qa::Answer>;

class QueryServer
{
public:
	QueryServer(EventLoop* loop, const EndPoint& addr)
		: codec_(loop)
	{
		server_ = make_tcp_server(loop, addr, "QueryServer");
		
		server_->set_connect_callback(
			std::bind(&QueryServer::on_connect, this, _1));
		server_->set_message_callback(
			std::bind(&ProtobufCodec::decode_read, &codec_, _1, _2, _3));

		codec_.set_dispatch_callback(
			std::bind(&ProtobufDispatcher::dispatch_message, &dispatcher_, _1, _2, _3));

		dispatcher_.register_message_cb<qa::Query>(
			std::bind(&QueryServer::on_query, this, _1, _2, _3));
		dispatcher_.register_message_cb<qa::Answer>(
			std::bind(&QueryServer::on_answer, this, _1, _2, _3));
	}

	void start()
	{
		server_->start();
	}

private:
	void on_connect(const TcpConnectionPtr& conn);

	void on_unknown_message(const TcpConnectionPtr& conn, const MessagePtr& mesg, TimeStamp);
	void on_query(const TcpConnectionPtr& conn, const QueryPtr& mesg, TimeStamp);
	void on_answer(const TcpConnectionPtr& conn, const AnswerPtr& mesg, TimeStamp);

private:
	TcpServerPtr server_;

	ProtobufDispatcher dispatcher_;
	ProtobufCodec codec_;
};

void QueryServer::on_connect(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "QueryServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");
}

void QueryServer::on_answer(const TcpConnectionPtr& conn, const AnswerPtr& mesg, TimeStamp)
{
	LOG(INFO) << "QueryServer::on_answer";
	LOG(INFO) << mesg->GetTypeName() << mesg->DebugString();
	
	conn->shutdown();
}

void QueryServer::on_query(const TcpConnectionPtr& conn, const QueryPtr& mesg, TimeStamp)
{
	LOG(INFO) << "QueryServer::on_query:";
	LOG(INFO) << mesg->GetTypeName() << mesg->DebugString();
	
	Answer answer;
	answer.set_id(1);
	answer.set_questioner("Anny Wang");
	answer.set_answerer("~~~");
	answer.add_solution("World!");
	
	codec_.endcode_send(conn, answer);

	conn->shutdown();
}

int main(int argc, char* argv[])
{
	set_min_log_severity(LOG_DEBUG);
	
	EventLoop loop;
	QueryServer server(&loop, EndPoint(1669));

	server.start();
	loop.loop();
}
