// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_ACCEPTOR_H_
#define ANT_ACCEPTOR_H_

#include "Macros.h"
#include "Channel.h"
#include "SocketFD.h"

#include <functional>

namespace annety
{
class EventLoop;
class EndPoint;

// Acceptor of incoming TCP connections
class Acceptor
{
public:
	using NewConnectCallback = std::function<void(int, const EndPoint&)>;

	Acceptor(EventLoop* loop, const EndPoint& listenAddr, bool reuseport);
	~Acceptor();
	
	void set_new_connect_callback(const NewConnectCallback& cb)
	{
		new_connect_cb_ = cb;
	}

	bool listenning() const { return listenning_; }
	void listen();

private:
	void handle_read();

private:
	EventLoop* owner_loop_;
	
	// listen socket
	SocketFD accept_socket_;
	Channel accept_channel_;
	
	NewConnectCallback new_connect_cb_;
	//int idle_fd_;

	bool listenning_;
};

}	// namespace annety

#endif	// ANT_ACCEPTOR_H_
