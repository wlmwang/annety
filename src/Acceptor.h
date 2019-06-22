// Refactoring: Anny Wang
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

class Acceptor
{
public:
	using NewConnectionCallback = std::function<void(SelectableFDPtr, const EndPoint&)>;

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
	EventLoop* owner_loop_{nullptr};
	bool listenning_{false};

	// listen socket
	std::unique_ptr<SelectableFD> accept_socket_;
	std::unique_ptr<Channel> accept_channel_;
	
	NewConnectionCallback new_connection_cb_;

	int idle_fd_;

	DISALLOW_COPY_AND_ASSIGN(Acceptor);
};

}	// namespace annety

#endif	// ANT_ACCEPTOR_H_
