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
					const Query* request,
					Answer* response,
					::google::protobuf::Closure* done)
	{
		LOG(INFO) << "QueryServiceImpl::Solve";
		
		response->set_id(1);
		response->set_questioner(request->questioner());
		response->set_answerer("Anny Wang");
		response->add_solution("World!!");

		done->Run();
	}
};

}	// namespace rpc
}	// namespace protobuf
}	// namespace examples

int main(int argc, char* argv[])
{
	EventLoop loop;
	
	QueryServiceImpl impl;
	ProtorpcServer server(&loop, EndPoint(1669));
	server.listen(&impl);
	server.start();

	loop.loop();

	google::protobuf::ShutdownProtobufLibrary();
}
