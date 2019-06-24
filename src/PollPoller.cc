// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "PollPoller.h"
#include "Logging.h"
#include "Channel.h"
#include "ScopedClearLastError.h"

#include <algorithm>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

namespace annety
{
PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {}

PollPoller::~PollPoller() = default;

Time PollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
	Poller::check_in_own_loop();

	LOG(TRACE) << "watching total fd count " << (long long)channels_.size();
	ScopedClearLastError last_error;

	int num = ::poll(pollfds_.data(), pollfds_.size(), timeout_ms);

	Time now(Time::now());
	if (num > 0) {
		LOG(TRACE) << num << " events happened";

		fill_active_channels(num, active_channels);
	} else if (num == 0) {
		LOG(TRACE) << "nothing happened";
	} else {
		// error happens
		if (errno != EINTR) {
			PLOG(ERROR) << "PollPoller::poll() failed";
		}
	}
	return now;
}

void PollPoller::fill_active_channels(int num, ChannelList* active_channels) const
{
	Poller::check_in_own_loop();
	
	PollFdList::const_iterator pfd = pollfds_.begin();
	for (; pfd != pollfds_.end() && num > 0; ++pfd) {
		if (pfd->revents > 0) {
			--num;
			ChannelMap::const_iterator ch = channels_.find(pfd->fd);
			DCHECK(ch != channels_.end());

			Channel* channel = ch->second;
			DCHECK(channel->fd() == pfd->fd);

			channel->set_revents(pfd->revents);
			active_channels->push_back(channel);
		}
	}
}

void PollPoller::update_channel(Channel* channel)
{
	Poller::check_in_own_loop();
	
	const int index = channel->index();
	LOG(TRACE) << "fd = " << channel->fd() 
		<< " events = " << channel->events() << " index = " << index;

	if (channel->index() < 0) {
		// a new one, add to pollfds_
		DCHECK(channels_.find(channel->fd()) == channels_.end());

		struct pollfd pfd;
		pfd.fd = channel->fd();
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;
		pollfds_.push_back(pfd);

		int idx = static_cast<int>(pollfds_.size())-1;
		channel->set_index(idx);
		channels_[pfd.fd] = channel;
	} else {
		// update existing one
		DCHECK(channels_.find(channel->fd()) != channels_.end());
		DCHECK(channels_[channel->fd()] == channel);

		int idx = channel->index();
		DCHECK(0 <= idx && idx < static_cast<int>(pollfds_.size()));

		struct pollfd& pfd = pollfds_[idx];
		DCHECK(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);

		// update
		pfd.fd = channel->fd();
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;

		// ignore this pollfd
		if (channel->is_none_event()) {
			pfd.fd = -channel->fd()-1;	// avoid fd=0
		}
	}
}

void PollPoller::remove_channel(Channel* channel)
{
	Poller::check_in_own_loop();

	int fd = channel->fd();
	LOG(TRACE) << "fd = " << fd;

	DCHECK(channels_.find(fd) != channels_.end());
	DCHECK(channels_[fd] == channel);
	DCHECK(channel->is_none_event());

	int idx = channel->index();
	DCHECK(0 <= idx && idx < static_cast<int>(pollfds_.size()));
	
	const struct pollfd& pfd = pollfds_[idx];
	DCHECK(pfd.fd == -fd-1 && pfd.events == channel->events());
	
	size_t n = channels_.erase(fd);
	DCHECK(n == 1);
	
	if (static_cast<size_t>(idx) == pollfds_.size()-1) {
		pollfds_.pop_back();
	} else {
		// O(1). erase container element
		int channel_at_end = pollfds_.back().fd;
		std::iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
		if (channel_at_end < 0) {
			channel_at_end = -channel_at_end-1;
		}
		channels_[channel_at_end]->set_index(idx);
		pollfds_.pop_back();
	}
}

}	// namespace annety
