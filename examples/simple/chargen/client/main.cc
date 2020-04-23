// By: wlmwang
// Date: Aug 04 2019

#include "client.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::set_min_log_severity(annety::LOG_DEBUG);

	annety::EventLoop loop;
	
	ChargenClient client(&loop, annety::EndPoint(1669));
	client.connect();

	loop.loop();
}
