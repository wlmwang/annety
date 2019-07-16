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

#include <signal.h> // for SIGTERM...

namespace annety
{
class EventLoop;
class Channel;

// Wrapper server with signal handler
//
// Notice:It be created only in main loop thread
class SignalServer
{
public:
	explicit SignalServer(EventLoop* loop);
	~SignalServer();

	void add_signal(int signo, SignalCallback cb);
	void del_signal(int signo);
	void reset_signal();

	// must be called in own loop thread(main thread)
	// FIXME.
	bool ismember_signal(int signo);

private:
	using SignalMap = std::map<int, SignalCallback>;

	void add_signal_in_own_loop(int signo, SignalCallback cb);
	void del_signal_in_own_loop(int signo);
	void reset_signal_in_own_loop();

	void handle_read();

private:
	EventLoop* owner_loop_{nullptr};

	SelectableFDPtr signal_socket_;
	std::unique_ptr<Channel> signal_channel_;

	bool calling_signal_functor_{false};

	SignalMap signals_;
};

}	// namespace annety

#endif	// ANT_SIGNAL_SERVER_H_
