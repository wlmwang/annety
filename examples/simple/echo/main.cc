// By: wlmwang
// Date: Aug 03 2019

#include "echo.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;

	EchoServer server(&loop, annety::EndPoint(1669));
	server.listen();
	
	loop.loop();
}
