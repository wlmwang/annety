// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "TcpServer.h"
#include "Logging.h"
#include "EventLoop.h"

#include <stdio.h>

using namespace annety;

void on_connect(const TcpConnectionPtr& conn)
{
	if (conn->connected()) {
		conn->set_tcp_nodelay(true);
	}
}

void on_message(const TcpConnectionPtr& conn, NetBuffer* buf, TimeStamp)
{
	// echo back
	conn->send(buf);
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: server <threads>\n");
	} else {
		int threads = atoi(argv[1]);
		
		LOG(INFO) << "PingPong server start. threads=" << threads;
		set_min_log_severity(LOG_WARNING);

		EventLoop loop;
		TcpServerPtr server = make_tcp_server(&loop, EndPoint(1669), "PingPongServer");
		server->set_connect_callback(on_connect);
		server->set_message_callback(on_message);

		if (threads > 1) {
			server->set_thread_num(threads);
		}
		server->start();
		
		loop.loop();
	}
}
