// By: wlmwang
// Date: Jun 17 2019

#include "PollPoller.h"
#include "Channel.h"
#include "Logging.h"
#include "ScopedClearLastError.h"

#include <algorithm>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

namespace annety
{
PollPoller::PollPoller(EventLoop* loop) : Poller(loop)
{
	CHECK(loop);
}

PollPoller::~PollPoller() = default;

TimeStamp PollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
	Poller::check_in_own_loop();

	DLOG(TRACE) << "PollPoller::poll is watching " << (long long)channels_.size() << " fd";

	ScopedClearLastError last_error;

	// Get a list of all triggered IO events
	int num = ::poll(listen_events_.data(), listen_events_.size(), timeout_ms);
	TimeStamp curr(TimeStamp::now());

	if (num > 0) {
		DLOG(TRACE) << "PollPoller::poll is having " << num << " events happened";

		fill_active_channels(num, active_channels);
	} else if (num == 0) {
		DLOG(TRACE) << "PollPoller::poll nothing was happened";
	} else {
		// error happens
		if (errno != EINTR) {
			PLOG(ERROR) << "PollPoller::poll a error was happened";
		}
	}
	return curr;
}

void PollPoller::update_channel(Channel* channel)
{
	Poller::check_in_own_loop();
	
	const int status = channel->status();
	DLOG(TRACE) << "PollPoller::update_channel fd = " << channel->fd() 
		<< " events = " << channel->events() << " status = " << status;

	if (status == kChannelPollInit || status == kChannelPollDeleted) {
		// Add a new or re-add a deleted(Disabled event) channel.
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

void PollPoller::remove_channel(Channel* channel)
{
	Poller::check_in_own_loop();

	int fd = channel->fd();
	DLOG(TRACE) << "PollPoller::remove_channel fd = " << fd;

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

void PollPoller::fill_active_channels(int num, ChannelList* active_channels) const
{
	Poller::check_in_own_loop();

	DLOG(TRACE) << "PollPoller::fill_active_channels is having " << num << " events happened";
	
	PollFdList::const_iterator vit = listen_events_.begin();
	for (; vit != listen_events_.end() && num > 0; ++vit) {
		if (vit->revents > 0) {
			--num;
			ChannelMap::const_iterator mit = channels_.find(vit->fd);
			DCHECK(mit != channels_.end());

			Channel* channel = mit->second;
			DCHECK(channel->fd() == vit->fd);

			channel->set_revents(vit->revents);
			active_channels->push_back(channel);
		}
	}
}

void PollPoller::update_poll_events(int operation, Channel* channel)
{
	Poller::check_in_own_loop();

	DLOG(TRACE) << "PollPoller::update_poll_events poll_ctl op = " 
		<< operation_to_string(operation) << " fd = " << channel->fd() 
		<< " event = { " << channel->events_to_string() << " }";

	PollFdList::iterator it = find_poll_events(channel->fd());

	switch (operation) {
	case kPollCtlAdd:
		DCHECK(it == listen_events_.end());

		struct pollfd pfd;
		pfd.fd = channel->fd();
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;
		listen_events_.push_back(pfd);
		break;
	
	case kPollCtlDel:
		DCHECK(it != listen_events_.end());

		listen_events_.erase(it);
		break;

	case kPollCtlMod:
		DCHECK(it != listen_events_.end());

		it->revents = 0;
		it->events = static_cast<short>(channel->events());
		break;
	}
}

std::vector<struct pollfd>::iterator PollPoller::find_poll_events(int fd)
{
	Poller::check_in_own_loop();

	PollFdList::iterator it = listen_events_.begin();
	for (; it != listen_events_.end(); ++it) {
		if (it->fd == fd) {
			break;
		}
	}

	return it;
}

}	// namespace annety
