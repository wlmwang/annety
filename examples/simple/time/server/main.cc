// By: wlmwang
// Date: Aug 03 2019

#include "server.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;

	TimeServer server(&loop, annety::EndPoint(1669));
	server.start();
	
	loop.loop();
}
