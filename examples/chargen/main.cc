// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "chargen.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::EventLoop loop;
	ChargenServer server(&loop, annety::EndPoint(1669));
	
	server.start();
	loop.loop();
}
