// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_CHANNEL_H_
#define ANT_CHANNEL_H_

#include "Macros.h"
#include "Time.h"

#include <memory>
#include <utility>

namespace annety {
class EventLoop;
class SelectableFD;

class Channel {
public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(Time)> ReadEventCallback;

	Channel(EventLoop* loop, SelectableFD* sfd);
	~Channel();

	void set_read_callback(ReadEventCallback cb) {
		read_callback_ = std::move(cb);
	}
	void set_write_callback(EventCallback cb) {
		write_callback_ = std::move(cb);
	}
	void set_close_callback(EventCallback cb) {
		close_callback_ = std::move(cb);
	}
	void set_error_callback(EventCallback cb) {
		error_callback_ = std::move(cb);
	}


	int fd() const;

	int events() const {
		return events_;
	}
	void set_revents(int revt) {
		revents_ = revt;
	}
	int revents() const {
		return revents_;
	}

	bool is_none_event() const {
		return events_ == kNoneEvent;
	}
	bool is_write_event() const {
		return events_ & kWriteEvent;
	}
	bool is_read_event() const {
		return events_ & kReadEvent;
	}
	
	void disable_all_event() {
		events_ = kNoneEvent;
		update();
	}
	void enable_read_event() {
		events_ |= kReadEvent;
		update();
	}
	void disable_read_event() {
		events_ &= ~kReadEvent;
		update();
	}
	void enable_write_event() {
		events_ |= kWriteEvent;
		update();
	}
	void disable_write_event() {
		events_ &= ~kWriteEvent;
		update();
	}
	
	void handle_event(Time receiveTime);

private:
	void update();
  
	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

private:
	EventLoop* owner_loop_;
	SelectableFD* select_fd_;

	int	events_;
	int revents_;

	bool eventHandling_;
	bool addedToLoop_;

	ReadEventCallback read_callback_;
	EventCallback write_callback_;
	EventCallback close_callback_;
	EventCallback error_callback_;

	DISALLOW_COPY_AND_ASSIGN(Channel);
};

}	// namespace annety

#endif	// 
