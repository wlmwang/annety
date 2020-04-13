// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_ACCEPTOR_H_
#define ANT_ACCEPTOR_H_

#include "Macros.h"
#include "CallbackForward.h"

#include <functional>
#include <memory>

namespace annety
{
class EventLoop;
class EndPoint;
class SelectableFD;
class Channel;

// Wrapper listen socket which used to accept connect sockets.
//
// This class owns the SelectableFD and Channel lifetime.
// *Not thread safe*, but they are all called in the own loop.
class Acceptor
{
public:
	using NewConnectCallback = std::function<void(SelectableFDPtr, const EndPoint&)>;

	Acceptor(EventLoop* loop, const EndPoint& addr, bool reuseport);
	~Acceptor();
	
	void set_new_connect_callback(const NewConnectCallback& cb)
	{
		new_connect_cb_ = cb;
	}
	bool is_listen() const
	{
		return listen_;
	}
	
	void listen();

private:
	void handle_read();

private:
	EventLoop* owner_loop_{nullptr};
	bool listen_{false};

	// listen socket
	SelectableFDPtr listen_socket_;
	std::unique_ptr<Channel> listen_channel_;
	
	NewConnectCallback new_connect_cb_;

	// A special file descriptor is used to solve the situation that the file 
	// descriptors are exhausted when accept().
	// By Marc Lehmann, author of libev.
	int idle_fd_;

	DISALLOW_COPY_AND_ASSIGN(Acceptor);
};

}	// namespace annety

#endif	// ANT_ACCEPTOR_H_
