// By: wlmwang
// Date: Jul 15 2019

#ifndef ANT_SIGNAL_SERVER_H_
#define ANT_SIGNAL_SERVER_H_

#include "Macros.h"
#include "CallbackForward.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <memory>
#include <functional>
#include <signal.h> // SIGTERM,...

namespace annety
{
class EventLoop;
class Channel;

// Signal server wrapper of OS signal.
// It can handle all signals except SIGKILL and SIGSTOP.
//
// Notice: It can only be created in main thread.
class SignalServer
{
public:
	explicit SignalServer(EventLoop* loop);
	~SignalServer();

	void add_signal(int signo, SignalCallback cb);
	void delete_signal(int signo);
	void ignore_signal(int signo);
	void revert_signal();

	// must be called in own loop thread(main thread)
	bool ismember_signal(int signo);

private:
	using SignalSet = std::unordered_set<int>;
	using SignalMap = std::unordered_map<int, SignalCallback>;

	void handle_read();
	void add_signal_in_own_loop(int signo, SignalCallback cb);
	void delete_signal_in_own_loop(int signo);
	void revert_signal_in_own_loop();

private:
	EventLoop* owner_loop_{nullptr};

	SelectableFDPtr signal_socket_;
	std::unique_ptr<Channel> signal_channel_;

	bool calling_signal_functor_{false};

	SignalMap signals_;
};

}	// namespace annety

#endif	// ANT_SIGNAL_SERVER_H_
