// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "times.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	// 屏蔽日志
	annety::set_min_log_severity(annety::LOG_INFO);
	
	annety::EventLoop loop;
	TimeClient client(&loop, annety::EndPoint(1669));
	
	client.connect();
	loop.loop();
}
