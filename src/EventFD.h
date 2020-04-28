// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_EVENT_FD_H_
#define ANT_EVENT_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

namespace annety
{
// An "eventfd object" that can be used as an event wait/notify mechanism by 
// user-space applications, and by the kernel to notify user-space applications 
// of events.
// The file descriptor may be monitored by select(2), poll(2), and epoll(7).
// It is simpler than the traditional pipe mechanism.
//
// Linux:
// The fds_ attribute is a file descriptor created by ::eventfd() - Linux 2.6.26
// fds_ is output, it will be monitored by the channel for readable events.
// fds_ is input, it will be sync written by external when the event is triggered.
//
// Others:
// The fds_[2] attribute is two file descriptors created by ::pipe().
// fds_[0] is output, it will be monitored by the channel for readable events.
// fds_[1] is input, it will be sync written by external when the event is triggered.
class EventFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	EventFD() = delete;
	EventFD(bool nonblock, bool cloexec);
	
	virtual ~EventFD() override;

	virtual ssize_t read(void *buf, size_t len) override;
	virtual ssize_t write(const void *buf, size_t len) override;

private:
#if !defined(OS_LINUX)
	int fds_[2];
#endif	// !defined(OS_LINUX)

	DISALLOW_COPY_AND_ASSIGN(EventFD);
};

}	// namespace annety

#endif	// _ANT_EVENT_FD_H_
