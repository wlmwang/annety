// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_TIMER_FD_H_
#define ANT_TIMER_FD_H_

#include "Macros.h"
#include "SelectableFD.h"
#include "TimeStamp.h"

#include <memory>

namespace annety
{
// A timer that delivers timer expiration notifications via a file descriptor.
//
// They provide an alternative to the use of setitimer(2) or timer_create(2), 
// with the advantage that the file descriptor may be monitored by select(2), 
// poll(2), and epoll(7).
//
// Linux:
// The fds_ attribute is a file descriptor created by ::timerfd_create() - Linux 2.6.27.
// fds_ is output, it will be monitored by the channel for readable events.
// fds_ is input, it will be sync written by external when the event is triggered.
//
// Others:
// Use EventFD(:pipe) to simulate `timerfd`. Provide the same timer expiration notification 
// that can pass the file descriptor.
class TimerFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	TimerFD() = delete;
	TimerFD(bool nonblock, bool cloexec);
	
	virtual ~TimerFD() override;

	virtual ssize_t read(void *buf, size_t len) override;
	virtual ssize_t write(const void *buf, size_t len) override;

	void reset(const TimeDelta& delta_ms);

private:
#if !defined(OS_LINUX)
	std::unique_ptr<SelectableFD> ev_;
#endif

	DISALLOW_COPY_AND_ASSIGN(TimerFD);
};

}	// namespace annety

#endif	// _ANT_TIMER_FD_H_
