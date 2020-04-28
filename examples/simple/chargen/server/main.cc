// By: wlmwang
// Date: Aug 04 2019

#include "server.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::set_min_log_severity(annety::LOG_DEBUG);
	
	annety::EventLoop loop;
	
	ChargenServer server(&loop, annety::EndPoint(1669));
	server.listen();

	loop.loop();
}
