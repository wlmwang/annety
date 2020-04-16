// By: wlmwang
// Date: Aug 03 2019

#include "echo.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::set_min_log_severity(annety::LOG_DEBUG);
	
	annety::EventLoop loop;
	EchoServer server(&loop, annety::EndPoint(1669));
	
	// io loop
	if (argc > 1) {
		server.set_thread_num(atoi(argv[1]));
	}

	server.start();
	loop.loop();
}
