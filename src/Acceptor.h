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

class Acceptor
{
public:
	using NewConnectionCallback = std::function<void(const SocketFD&, const EndPoint&)>;

	Acceptor(EventLoop* loop, const EndPoint& addr, bool reuseport);
	~Acceptor();
	
	void set_new_connection_callback(const NewConnectionCallback& cb)
	{
		new_connection_cb_ = cb;
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
	
	NewConnectionCallback new_connection_cb_;

	bool listenning_{false};

	DISALLOW_COPY_AND_ASSIGN(Acceptor);
};

}	// namespace annety

#endif	// ANT_ACCEPTOR_H_
