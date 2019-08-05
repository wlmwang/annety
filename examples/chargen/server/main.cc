// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "chargen.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;
	ChargenServer server(&loop, annety::EndPoint(1669));
	
	// 屏蔽日志
	annety::set_min_log_severity(annety::LOG_INFO);

	server.start();
	loop.loop();
}
