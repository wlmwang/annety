// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EPOLL_POLLER_H_
#define ANT_EPOLL_POLLER_H_

#if defined(OS_LINUX)
#include "Macros.h"
#include "Poller.h"
#include "TimeStamp.h"

#include <vector>
#include <poll.h>
#include <sys/epoll.h>

namespace annety
{
class EPollPoller : public Poller
{
public:
	EPollPoller(EventLoop* loop);
	~EPollPoller() override;

	TimeStamp poll(int timeout_ms, ChannelList* active_channels) override;

	void update_channel(Channel* channel) override;
	void remove_channel(Channel* channel) override;

private:
	using EventList = std::vector<struct epoll_event>;

	static const int kInitEventListSize = 16;
	static const char* operation_to_string(int op);
	
	void fill_active_channels(int num, ChannelList* active_channels) const;
	void update(int operation, Channel* channel);

private:
	int epollfd_;
	EventList events_;

	DISALLOW_COPY_AND_ASSIGN(EPollPoller);
};

}	// namespace annety

#endif	// OS_LINUX

#endif	// ANT_EPOLL_POLLER_H_
