// By: wlmwang
// Date: Aug 04 2019

#include "TcpServer.h"
#include "Logging.h"
#include "EventLoop.h"

#include <stdio.h>

using namespace annety;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

void on_connect(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "PingpongServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	conn->set_tcp_nodelay(true);
}

void on_close(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "PingpongServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";
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
		
		EventLoop loop;
		TcpServerPtr server = make_tcp_server(&loop, EndPoint(1669), "PingPongServer");
		server->set_connect_callback(on_connect);
		server->set_close_callback(on_close);
		server->set_message_callback(on_message);

		if (threads > 1) {
			server->set_thread_num(threads);
		}
		server->listen();
		
		loop.loop();
	}
}
