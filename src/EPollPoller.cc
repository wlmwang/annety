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
// the constants of poll(2) and epoll(4) are expected to be the same.
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

static_assert(EPOLL_CTL_ADD == kPollCtlAdd, "epoll uses same flag values as const");
static_assert(EPOLL_CTL_DEL == kPollCtlDel, "epoll uses same flag values as const");
static_assert(EPOLL_CTL_MOD == kPollCtlMod, "epoll uses same flag values as const");

// epoll_create1() was added to the kernel in version 2.6.27. Library support 
// is provided in glibc starting with version 2.9.
EPollPoller::EPollPoller(EventLoop* loop)
	: Poller(loop)
	, epollfd_(::epoll_create1(EPOLL_CLOEXEC))
	, active_events_(kInitEventListSize)
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

	DLOG(TRACE) << "EPollPoller::poll is watching " 
		<< static_cast<uint64_t>(channels_.size()) << " fd";
	
	ScopedClearLastError last_error;

	// Get a list of all triggered IO events
	int num = ::epoll_wait(epollfd_, active_events_.data(), active_events_.size(), timeout_ms);
	TimeStamp curr(TimeStamp::now());
	
	if (num > 0) {
		fill_active_channels(num, active_channels);

		// Increase event queue length
		if (static_cast<size_t>(num) == active_events_.size()) {
			active_events_.resize(active_events_.size()*2);
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

void EPollPoller::update_channel(Channel* channel)
{
	Poller::check_in_own_loop();

	const int status = channel->status();
	DLOG(TRACE) << "EPollPoller::update_channel fd = " << channel->fd() 
		<< " events = " << channel->events() << " status = " << status;
	
	if (status == kChannelPollInit || status == kChannelPollDeleted) {
		// Add a new or re-add a deleted(Disabled event) channel,
		if (status == kChannelPollInit) {
			// Ensure the new one
			DCHECK(channels_.find(channel->fd()) == channels_.end());
			channels_[channel->fd()] = channel;
		} else {
			// Ensure the existing one
			DCHECK(channels_.find(channel->fd()) != channels_.end());
			DCHECK(channels_[channel->fd()] == channel);
		}

		channel->set_status(kChannelPollAdded);
		update_poll_events(kPollCtlAdd, channel);
	} else {
		// Update existing one
		DCHECK(status == kChannelPollAdded);

		// Ensure the existing one
		DCHECK(channels_.find(channel->fd()) != channels_.end());
		DCHECK(channels_[channel->fd()] == channel);

		if (channel->is_none_event()) {
			// Disabled event
			update_poll_events(kPollCtlDel, channel);
			channel->set_status(kChannelPollDeleted);
		} else {
			// Modify event
			update_poll_events(kPollCtlMod, channel);
		}
	}
}

void EPollPoller::remove_channel(Channel* channel)
{
	Poller::check_in_own_loop();

	int fd = channel->fd();
	DLOG(TRACE) << "EPollPoller::remove_channel fd = " << fd;

	// Ensure the existing one
	DCHECK(channels_.find(fd) != channels_.end());
	DCHECK(channels_[fd] == channel);
	DCHECK(channel->is_none_event());

	int status = channel->status();
	DCHECK(status == kChannelPollAdded || status == kChannelPollDeleted);
	
	size_t n = channels_.erase(fd);
	DCHECK(n == 1);

	if (status == kChannelPollAdded) {
		update_poll_events(kPollCtlDel, channel);
	}
	channel->set_status(kChannelPollInit);
}

void EPollPoller::fill_active_channels(int num, ChannelList* active_channels) const
{
	Poller::check_in_own_loop();

	DLOG(TRACE) << "EPollPoller::fill_active_channels is having " << num << " events happened";

	DCHECK(static_cast<size_t>(num) <= active_events_.size());
	for (int i = 0; i < num; ++i) {
		Channel* channel = static_cast<Channel*>(active_events_[i].data.ptr);

#if DCHECK_IS_ON()
		ChannelMap::const_iterator it = channels_.find(channel->fd());
		DCHECK(it != channels_.end());
		DCHECK(it->second == channel);
#endif	// DCHECK_IS_ON()
		
		channel->set_revents(active_events_[i].events);
		active_channels->push_back(channel);
	}
}

void EPollPoller::update_poll_events(int operation, Channel* channel)
{
	Poller::check_in_own_loop();

	struct epoll_event event;
	::memset(&event, 0, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;

	DLOG(TRACE) << "EPollPoller::update_poll_events epoll_ctl op = " 
		<< operation_to_string(operation) << " fd = " << channel->fd() 
		<< " event = { " << channel->events_to_string() << " }";
	
	if (::epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
		if (operation == kPollCtlDel) {
			LOG(ERROR) << "EPollPoller::update_poll_events epoll_ctl op =" 
				<< operation_to_string(operation) << " fd =" << channel->fd();
		} else {
			LOG(FATAL) << "EPollPoller::update_poll_events epoll_ctl op =" 
				<< operation_to_string(operation) << " fd =" << channel->fd();
		}
	}
}

}	// namespace annety

#else
namespace annety
{
// FIXME: suppression may cause "has no symbols" warnings for some compilers.
void ALLOW_UNUSED_TYPE suppress_no_symbols_warning()
{
	NOTREACHED();
}

}	// namespace annety

#endif	// OS_LINUX
