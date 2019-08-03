// Refactoring: Anny Wang
// Date: Aug 03 2019

#include "discard.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;
	DiscardServer server(&loop, annety::EndPoint(1669));
	
	server.start();
	loop.loop();
}
