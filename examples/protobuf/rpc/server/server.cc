#include "EventLoop.h"
#include "Logging.h"
#include "protorpc/ProtorpcServer.h"

#include "Query.pb.h"

#include <unistd.h>

using namespace annety;
using namespace examples::protobuf::rpc;

namespace examples
{
namespace protobuf
{
namespace rpc
{
class QueryServiceImpl : public QueryService
{
public:
	virtual void Solve(::google::protobuf::RpcController* controller,
					const QueryRequest* request, QueryResponse* response,
					::google::protobuf::Closure* done)
	{
		LOG(INFO) << "QueryServiceImpl::Solve";
		response->set_solved(true);
		response->set_checkerboard("1234567");
		done->Run();
	}
};

}	// namespace rpc
}	// namespace protobuf
}	// namespace examples

int main(int argc, char* argv[])
{
	set_min_log_severity(LOG_DEBUG);
	
	QueryServiceImpl impl;

	EventLoop loop;
	
	ProtorpcServer server(&loop, EndPoint(1669));
	server.listen(&impl);
	server.start();

	loop.loop();

	google::protobuf::ShutdownProtobufLibrary();
}
