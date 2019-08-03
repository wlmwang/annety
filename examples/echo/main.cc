// Refactoring: Anny Wang
// Date: Aug 03 2019

#include "echo.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;
	EchoServer server(&loop, annety::EndPoint(1669));

	LOG(INFO) << "server listen on ";

	server.start();
	loop.loop();
}
