// By: wlmwang
// Date: Aug 03 2019

#include "daytime.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;
	DaytimeServer server(&loop, annety::EndPoint(1669));
	
	server.start();
	loop.loop();
}
