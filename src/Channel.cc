// By: wlmwang
// Date: Jun 17 2019

#include "Channel.h"
#include "SelectableFD.h"
#include "EventLoop.h"
#include "Logging.h"

#include <sstream>
#include <utility>
#include <poll.h>

namespace annety
{
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, SelectableFD* sfd)
	: owner_loop_(loop)
	, select_fd_(sfd) {}

Channel::~Channel()
{
	DCHECK(!event_handling_);
	DCHECK(!added_to_loop_);

	if (owner_loop_->is_in_own_loop()) {
		// channel must has removed
		DCHECK(!owner_loop_->has_channel(this));
	}
}

int Channel::fd() const
{
	return select_fd_->internal_fd();
}

void Channel::handle_event(TimeStamp receive_tm)
{
	owner_loop_->check_in_own_loop();
	
	LOG(TRACE) << "Channel::handle_event is handling event begin " << revents_to_string();
	event_handling_ = true;

	// hup event
	// FIXME: revents_ & POLLOUT? (::connect could be trigger?)
	if ((revents_ & POLLHUP) && (!(revents_ & POLLIN) /*|| !(revents_ & POLLOUT)*/)) {
		LOG_IF(WARNING, logging_hup_) << "Channel::handle_event fd = " << fd() 
			<< " POLLHUP event has received";
		
		if (close_cb_) close_cb_();
	}
	// error
	if (revents_ & (POLLERR | POLLNVAL)) {
		LOG_IF(WARNING, revents_ & POLLNVAL) << "Channel::handle_event fd = " << fd() 
			<< " POLLNVAL event has received";
		
		if (error_cb_) error_cb_();
	}
	// readable
	if (revents_ & (POLLIN | POLLPRI | POLLHUP)) {
		if (read_cb_) read_cb_(receive_tm);
	}
	// writeable
	if (revents_ & POLLOUT) {
		if (write_cb_) write_cb_();
	}

	event_handling_ = false;
	LOG(TRACE) << "Channel::handle_event event was handled finish";
}

void Channel::remove()
{
	owner_loop_->check_in_own_loop();

	// channel must has disable_all_event
	DCHECK(is_none_event());
	
	added_to_loop_ = false;
	owner_loop_->remove_channel(this);
}

void Channel::update()
{
	owner_loop_->check_in_own_loop();

	added_to_loop_ = true;
	owner_loop_->update_channel(this);
}

std::string Channel::revents_to_string() const
{
	return events_to_string(fd(), revents_);
}

std::string Channel::events_to_string() const
{
	return events_to_string(fd(), events_);
}

std::string Channel::events_to_string(int fd, int ev)
{
	std::ostringstream oss;

	oss << "[fd=" << fd << " event: ";
	if (ev & POLLIN) {
		oss << "IN ";
	}
	if (ev & POLLPRI) {
		oss << "PRI ";
	}
	if (ev & POLLOUT) {
		oss << "OUT ";
	}
	if (ev & POLLHUP) {
		oss << "HUP ";
	}
#if !defined(OS_MACOSX)
	if (ev & POLLRDHUP) {
		oss << "RDHUP ";
	}
#endif	// OS_MACOSX
	if (ev & POLLERR) {
		oss << "ERR ";
	}
	if (ev & POLLNVAL) {
		oss << "NVAL ";
	}
	oss << "]";

	return oss.str();
}

}	// namespace annety
