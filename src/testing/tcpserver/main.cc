
#include "TcpServer.h"
#include "EventLoop.h"
#include "Logging.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	EventLoop loop;

	TcpServerPtr srv = make_tcp_server(&loop, EndPoint(1669), "TestTcpServer");

	// register connect handle
	srv->set_connect_callback([](const TcpConnectionPtr& conn) {
		conn->send("\r\n********************\r\n");
		conn->send("welcome to TestServer\r\n");
		conn->send("********************\r\n");

		LOG(INFO) << conn->local_addr().to_ip_port() << " <- "
			   << conn->peer_addr().to_ip_port() << " s is "
			   << (conn->connected() ? "UP" : "DOWN");
	});
	
	// register message handle
	srv->set_message_callback([&](const TcpConnectionPtr& conn, NetBuffer* buf, TimeStamp time) {
		std::string message(buf->taken_as_string());
	
		LOG(INFO) << conn->name() << " echo " << (int)message.size() << " bytes, "
			<< "data received at " << time;

		// echo back
		conn->send(message);
	});
	srv->start();

	loop.loop();
}
