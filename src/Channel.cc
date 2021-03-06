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
// Same flag values as Poller.h
namespace {
const int kChannelPollInit = -1;
}	// anonymous namespace

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, SelectableFD* sfd)
	: owner_loop_(loop)
	, select_fd_(sfd)
	, status_(kChannelPollInit)
{
	CHECK(loop && sfd);

	DLOG(TRACE) << "Channel::Channel the channel fd=" 
		<< select_fd_->internal_fd() << " is constructing";
}

Channel::~Channel()
{
	// Object cannot be destructed during event handling, and 
	// it has been removed from the event loop.
	DCHECK(!handling_event_);
	DCHECK(!added_to_loop_);

	if (owner_loop_->is_in_own_loop()) {
		// channel must has removed.
		DCHECK(!owner_loop_->has_channel(this));
	}

	DLOG(TRACE) << "Channel::~Channel the channel fd=" 
		<< select_fd_->internal_fd() << " is destructing";
}

int Channel::fd() const
{
	return select_fd_->internal_fd();
}

void Channel::handle_event(TimeStamp received_ms)
{
	owner_loop_->check_in_own_loop();
	
	DLOG(TRACE) << "Channel::handle_event is handling event begin " 
		<< revents_to_string();
	handling_event_ = true;

	// POLLHUP: Hang up (output only).
	//
	// !(revents_ & POLLIN) means:
	// 1. Let `read_cb_` handle normal connection closure.
	// 2. HUP in writable event, indicating that you are writing a closed connection.
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
		LOG_IF(WARNING, logging_hup_) << "Channel::handle_event fd = " << fd() 
			<< " POLLHUP event has received";
		
		if (close_cb_) close_cb_();
	}

	// POLLERR: Error condition (output only).
	// POLLNVAL: Invalid request: fd not open (output only).
	if (revents_ & (POLLERR | POLLNVAL)) {
		LOG_IF(WARNING, revents_ & POLLNVAL) << "Channel::handle_event fd = " << fd() 
			<< " POLLNVAL event has received";
		
		if (error_cb_) error_cb_();
	}

	// POLLIN: There is data to read.
	// POLLPRI: There is urgent data to read.
	// POLLRDHUP: Stream socket peer closed connection, or shutdown writing half 
	// of connection.  --- since Linux 2.6.17.
	if (revents_ & (POLLIN | POLLPRI /*| POLLRDHUP*/)) {
		if (read_cb_) read_cb_(received_ms);
	}

	// POLLOUT: Writing now will not block.
	if (revents_ & POLLOUT) {
		if (write_cb_) write_cb_();
	}

	handling_event_ = false;
	DLOG(TRACE) << "Channel::handle_event event was handled finish";
}

void Channel::remove()
{
	owner_loop_->check_in_own_loop();

	// Channel must has disable_all_event
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
