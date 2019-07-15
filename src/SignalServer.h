// Refactoring: Anny Wang
// Date: Jul 15 2019

#ifndef ANT_SIGNAL_SERVER_H_
#define ANT_SIGNAL_SERVER_H_

#include "Macros.h"
#include "CallbackForward.h"

#include <map>
#include <utility>
#include <memory>
#include <functional>
#include <signal.h>		// for SIGTERM...

namespace annety
{
class EventLoop;
class Channel;

class SignalServer
{
public:
	using SignalCallback = std::function<void()>;
	
	explicit SignalServer(EventLoop* loop);
	~SignalServer();

	void add_signal(int signo, SignalCallback cb);

private:
	using SignalMap = std::map<int, SignalCallback>;

	void add_signal_in_own_loop(int signo, SignalCallback cb);
	void handle_read();

private:
	EventLoop* owner_loop_{nullptr};

	SelectableFDPtr signal_socket_;
	std::unique_ptr<Channel> signal_channel_;

	SignalMap signals_;
};

}	// namespace annety

#endif	// ANT_SIGNAL_SERVER_H_
