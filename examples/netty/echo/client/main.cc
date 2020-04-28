// By: wlmwang
// Date: Aug 04 2019

#include "client.h"
#include "Logging.h"

int main(int argc, char* argv[])
{
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
