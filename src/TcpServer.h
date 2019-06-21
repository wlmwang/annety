// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TCP_SERVER_H_
#define ANT_TCP_SERVER_H_

#include "Macros.h"
#include "CallbackForward.h"

#include <string>
#include <memory>
#include <map>

namespace annety
{
class SocketFD;
class EndPoint;
class Acceptor;
class EventLoop;

class TcpServer
{
using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

public:
	enum Option
	{
		kNoReusePort,
		kReusePort,
	};

	TcpServer(EventLoop* loop,
			  const EndPoint& addr,
			  const std::string& name = "",
			  Option option = kNoReusePort);

	~TcpServer();

	const std::string& ip_port() const { return ip_port_; }
	const std::string& name() const { return name_; }
	EventLoop* get_owner_loop() const { return owner_loop_; }

	void start();

private:
	void new_connection(const SocketFD& conn_sfd, const EndPoint& peerAddr);

private:
	EventLoop* owner_loop_{nullptr};
	const std::string ip_port_;
	const std::string name_;

	int started_{0};
	std::unique_ptr<Acceptor> acceptor_;

	int next_connId_{0};
	ConnectionMap connections_;

	DISALLOW_COPY_AND_ASSIGN(TcpServer);
};

}	// namespace annety

#endif	// ANT_TCP_SERVER_H_
