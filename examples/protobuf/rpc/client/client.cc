// By: wlmwang
// Date: Oct 18 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "protorpc/ProtorpcChannel.h"

#include "Query.pb.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class QueryClient
{
public:
	QueryClient(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, channel_(new ProtorpcChannel(loop))
		, stub_(channel_.get())
	{
		client_ = make_tcp_client(loop, addr, "QueryClient");

		client_->set_connect_callback(
			std::bind(&QueryClient::new_connection, this, _1));
		client_->set_close_callback(
			std::bind(&QueryClient::remove_connection, this, _1));
		client_->set_message_callback(
			std::bind(&ProtorpcChannel::recv, channel_.get(), _1, _2, _3));
	}

	void connect()
	{
		client_.connect();
	}

	void finish(Answer* resp);

private:
	void new_connection(const TcpConnectionPtr&);
	void remove_connection(const TcpConnectionPtr&);

private:
	EventLoop* loop_;
	TcpClientPtr client_;
	ProtorpcChannelPtr channel_;

	QueryService::Stub stub_;
};

void QueryClient::new_connection(const TcpConnectionPtr& conn)
{
	DLOG(TRACE) << "QueryClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	channel_->attach_connection(conn);

	Answer* res = new Answer();
	Query req;
	req->set_id(1);
	req->set_questioner("Anny Wang");
	req->add_question("Hello??");

	stub->Solve(nullptr, &req, res, NewCallback(this, &QueryClient::finish, res));
	// Will call ProtorpcChannel::CallMethod() callback.
}

void QueryClient::remove_connection(const TcpConnectionPtr& conn)
{
	DLOG(TRACE) << "QueryClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";

	channel_->detach_connection();
	loop_->quit();
}

void QueryClient::finish(Answer* res)
{
	LOG(INFO) << "QueryClient:result - " << res->DebugString();

	client_->disconnect();
}

int main(int argc, char* argv[])
{
	EventLoop loop;

	QueryClient client(&loop, EndPoint(1669));
	client.connect();

	loop.loop();

	google::protobuf::ShutdownProtobufLibrary();
}
