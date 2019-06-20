// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EPollPoller.h"
#include "Logging.h"
#include "Channel.h"
#include "ScopedClearLastError.h"

#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace annety
{
// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
	: Poller(loop),
	  epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
	  events_(kInitEventListSize)
{
	CHECK(epollfd_ >= 0) << "epoll_create1 failed";
}

EPollPoller::~EPollPoller()
{
	CHECK(::close(epollfd_) == 0);
}

Time EPollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
	LOG(TRACE) << "watching total fd count " << (long long)channels_.size();
	ScopedClearLastError last_error();

	int num = ::epoll_wait(epollfd_, events_.data(), events_.size(), timeout_ms);

	Time now(Time::now());
	if (num > 0) {
		LOG(TRACE) << num << " events happened";

		fill_active_channels(num, active_channels);

		if (static_cast<size_t>(num) == events_.size()) {
			events_.resize(events_.size()*2);
		}
	} else if (num == 0) {
		LOG(TRACE) << "nothing happened";
	} else {
		// error happens, log uncommon ones
		if (errno != EINTR) {
			PLOG(ERROR) << "epoll_wait() failed";
		}
	}
	return now;
}

void EPollPoller::fill_active_channels(int num,
									ChannelList* active_channels) const
{
	DCHECK(static_cast<size_t>(num) <= events_.size());
	
	for (int i = 0; i < num; ++i) {
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

#if DCHECK_IS_ON()
		int fd = channel->fd();
		ChannelMap::const_iterator it = channels_.find(fd);
		DCHECK(it != channels_.end());
		DCHECK(it->second == channel);
#endif	// DCHECK_IS_ON()
		
		channel->set_revents(events_[i].events);
		active_channels->push_back(channel);
	}
}

void EPollPoller::update_channel(Channel* channel)
{
	Poller::check_in_own_thread();

	const int index = channel->index();
	LOG(TRACE) << "fd = " << channel->fd() 
		<< " events = " << channel->events() << " index = " << index;
	
	if (index == kNew || index == kDeleted) {
		// a new one, add with EPOLL_CTL_ADD
		int fd = channel->fd();
		if (index == kNew) {
			DCHECK(channels_.find(fd) == channels_.end());
			channels_[fd] = channel;
		} else {
			// index == kDeleted
			DCHECK(channels_.find(fd) != channels_.end());
			DCHECK(channels_[fd] == channel);
		}

		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD, channel);
	} else {
		// update existing one with EPOLL_CTL_MOD/DEL
		int fd = channel->fd();
		(void)fd;
		DCHECK(channels_.find(fd) != channels_.end());
		DCHECK(channels_[fd] == channel);
		DCHECK(index == kAdded);
		if (channel->is_none_event()) {
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		} else {
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

void EPollPoller::remove_channel(Channel* channel)
{
	Poller::check_in_own_thread();

	int fd = channel->fd();
	LOG(TRACE) << "fd = " << fd;

	DCHECK(channels_.find(fd) != channels_.end());
	DCHECK(channels_[fd] == channel);
	DCHECK(channel->is_none_event());

	int index = channel->index();
	DCHECK(index == kAdded || index == kDeleted);

	// 删除 channel 句柄
	size_t n = channels_.erase(fd);
	(void)n;
	DCHECK(n == 1);

	if (index == kAdded) {
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
	struct epoll_event event;
	::memset(&event, 0, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;
	int fd = channel->fd();

	LOG(TRACE) << "epoll_ctl op = " << operation_to_string(operation)
		<< " fd = " << fd << " event = { " << channel->events_to_string() << " }";
	
	if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
		if (operation == EPOLL_CTL_DEL) {
			LOG(ERROR) << "epoll_ctl op =" << operation_to_string(operation) << " fd =" << fd;
		} else {
			LOG(FATAL) << "epoll_ctl op =" << operation_to_string(operation) << " fd =" << fd;
		}
	}
}

const char* EPollPoller::operation_to_string(int op)
{
	switch (op) {
	case EPOLL_CTL_ADD:
		return "ADD";
	case EPOLL_CTL_DEL:
		return "DEL";
	case EPOLL_CTL_MOD:
		return "MOD";
	default:
		DCHECK(false && "ERROR op");
		return "Unknown Operation";
	}
}

}	// namespace annety
