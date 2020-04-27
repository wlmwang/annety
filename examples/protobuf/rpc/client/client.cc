// By: wlmwang
// Date: Oct 18 2019

#include "TcpClient.h"
#include "EventLoop.h"
#include "protorpc/ProtorpcClient.h"

#include "Query.pb.h"

using namespace annety;
using Closure = ::google::protobuf::Closure;

void solve(QueryService::Stub* stub, Query* req, Answer* res, Closure* done)
{
	req->set_id(1);
	req->set_questioner("Anny Wang");
	req->add_question("Hello??");

	// Will call ProtorpcChannel::CallMethod() callback.
	stub->Solve(nullptr, req, res, done);
}

void solved(Answer* res)
{
	LOG(INFO) << "QueryClient:result - " << res->DebugString();
}

int main(int argc, char* argv[])
{
	{
		EventLoop loop;
		ProtorpcClient<Query, Answer, QueryService> client(&loop, EndPoint(1669));
		client.call(solve, solved);
		loop.loop();
	}

	{
		EventLoop loop;
		ProtorpcClient<Query, Answer, QueryService> client(&loop, EndPoint(1669));
		client.call(solve, solved);
		loop.loop();
	}

	google::protobuf::ShutdownProtobufLibrary();
}
