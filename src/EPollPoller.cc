// By: wlmwang
// Date: Jun 17 2019

#include "EPollPoller.h"
#include "Logging.h"
#include "Channel.h"
#include "ScopedClearLastError.h"

#if defined(OS_LINUX)
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace annety
{
namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

// the constants of poll(2) and epoll(4) are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

// epoll_create1() was added to the kernel in version 2.6.27. Library support 
// is provided in glibc starting with version 2.9.
EPollPoller::EPollPoller(EventLoop* loop)
	: Poller(loop)
	, epollfd_(::epoll_create1(EPOLL_CLOEXEC))
	, events_(kInitEventListSize)
{
	PCHECK(epollfd_ >= 0) << "epoll_create1 failed";
}

EPollPoller::~EPollPoller()
{
	PCHECK(::close(epollfd_) == 0);
}

TimeStamp EPollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
	Poller::check_in_own_loop();

	DLOG(TRACE) << "EPollPoller::poll is watching " << (long long)channels_.size() << " fd";
	
	ScopedClearLastError last_error;

	// Get a list of all triggered IO events
	int num = ::epoll_wait(epollfd_, events_.data(), events_.size(), timeout_ms);
	TimeStamp curr(TimeStamp::now());
	
	if (num > 0) {
		fill_active_channels(num, active_channels);

		// Increase event queue length
		if (static_cast<size_t>(num) == events_.size()) {
			events_.resize(events_.size()*2);
		}
	} else if (num == 0) {
		DLOG(TRACE) << "EPollPoller::poll nothing was happened";
	} else {
		// error happens
		if (errno != EINTR) {
			PLOG(ERROR) << "EPollPoller::poll a error was happened";
		}
	}

	return curr;
}

void EPollPoller::fill_active_channels(int num, ChannelList* active_channels) const
{
	Poller::check_in_own_loop();

	DLOG(TRACE) << "EPollPoller::fill_active_channels is having " << num << " events happened";

	DCHECK(static_cast<size_t>(num) <= events_.size());
	for (int i = 0; i < num; ++i) {
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

#if DCHECK_IS_ON()
		ChannelMap::const_iterator it = channels_.find(channel->fd());
		DCHECK(it != channels_.end());
		DCHECK(it->second == channel);
#endif	// DCHECK_IS_ON()
		
		channel->set_revents(events_[i].events);
		active_channels->push_back(channel);
	}
}

void EPollPoller::update_channel(Channel* channel)
{
	Poller::check_in_own_loop();

	const int index = channel->index();
	DLOG(TRACE) << "EPollPoller::update_channel fd = " << channel->fd() 
		<< " events = " << channel->events() << " index = " << index;
	
	if (index == kNew || index == kDeleted) {
		// Add a new or re-add a deleted(Disabled event) channel
		// With EPOLL_CTL_ADD
		int fd = channel->fd();
		if (index == kNew) {
			// Ensure the new one
			DCHECK(channels_.find(fd) == channels_.end());
			channels_[fd] = channel;
		} else {
			// Ensure the existing one
			DCHECK(channels_.find(fd) != channels_.end());
			DCHECK(channels_[fd] == channel);
		}

		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD, channel);
	} else {
		// Update existing one with EPOLL_CTL_MOD/DEL
		DCHECK(index == kAdded);

		int fd = channel->fd();
		DCHECK(channels_.find(fd) != channels_.end());
		DCHECK(channels_[fd] == channel);

		if (channel->is_none_event()) {
			// Disabled event
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		} else {
			// Modify event
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

void EPollPoller::remove_channel(Channel* channel)
{
	Poller::check_in_own_loop();

	int fd = channel->fd();
	DLOG(TRACE) << "PollPoller::remove_channel fd = " << fd;

	DCHECK(channels_.find(fd) != channels_.end());
	DCHECK(channels_[fd] == channel);
	DCHECK(channel->is_none_event());

	int index = channel->index();
	DCHECK(index == kAdded || index == kDeleted);
	
	size_t n = channels_.erase(fd);
	DCHECK(n == 1);

	if (index == kAdded) {
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
	Poller::check_in_own_loop();

	struct epoll_event event;
	::memset(&event, 0, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;

	DLOG(TRACE) << "EPollPoller::update epoll_ctl op = " << operation_to_string(operation)
		<< " fd = " << channel->fd() << " event = { " << channel->events_to_string() << " }";
	
	if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
		if (operation == EPOLL_CTL_DEL) {
			LOG(ERROR) << "EPollPoller::update epoll_ctl op =" << operation_to_string(operation) 
					<< " fd =" << channel->fd();
		} else {
			LOG(FATAL) << "EPollPoller::update epoll_ctl op =" << operation_to_string(operation) 
					<< " fd =" << channel->fd();
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
		NOTREACHED();
		return "Unknown Operation";
	}
}

}	// namespace annety

#else
namespace annety {
// FIXME: suppression may cause "has no symbols" warnings for some compilers.
void ALLOW_UNUSED_TYPE suppress_no_symbols_warning()
{
	NOTREACHED();
}

}	// namespace annety

#endif	// OS_LINUX
