// By: wlmwang
// Date: Aug 04 2019

#include "echo.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
	annety::set_min_log_severity(annety::LOG_DEBUG);

	if (argc < 1) {
		printf("Usage: %s [msg_size]\n", argv[0]);
	} else {
		int size = 256;
		if (argc > 1) {
			size = atoi(argv[1]);
		}

		annety::EventLoop loop;
		
		EchoClient client(&loop, annety::EndPoint(1669), size);
		client.connect();

		loop.loop();
	}
}
