
#include "TcpClient.h"
#include "EventLoop.h"
#include "Logging.h"

#include <iostream>

using namespace annety;
using namespace std;

int main(int argc, char* argv[])
{
	EventLoop loop;

	TcpClientPtr crv = make_tcp_client(&loop, EndPoint(1669), "TestTcpClient");

	// register connect handle
	crv->set_connect_callback([](const TcpConnectionPtr& conn) {
		LOG(INFO) << conn->local_addr().to_ip_port() << " <- "
			   << conn->peer_addr().to_ip_port() << " s is "
			   << (conn->connected() ? "UP" : "DOWN");
	});
	
	// register message handle
	crv->set_message_callback([&](const TcpConnectionPtr& conn, NetBuffer* buf, TimeStamp time) {
		std::string message(buf->taken_as_string());
		
		LOG(INFO) << conn->name() << " echo " << (int)message.size() << " bytes, "
			<< "data received at " << time;

		LOG(INFO) << "crv:" << message;
	});
	crv->connect();

	loop.loop();
}
