// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_CHANNEL_H_
#define ANT_CHANNEL_H_

#include "Macros.h"
#include "TimeStamp.h"

#include <string>
#include <atomic>
#include <memory>
#include <utility>
#include <functional>

namespace annety
{
class EventLoop;
class SelectableFD;

// A selectable IO channel.
// It acts as a combination of IO Multiplexing and file descriptors.
//
// This class does not owns the EventLoop and SelectableFD lifetime.
// File descriptor was wrapped which could be a socket, eventfd, timerfd or signalfd.
class Channel
{
public:
	using EventCallback = std::function<void()>;
	using ReadEventCallback = std::function<void(TimeStamp)>;

	void set_read_callback(ReadEventCallback cb)
	{
		read_cb_ = std::move(cb);
	}
	void set_write_callback(EventCallback cb)
	{
		write_cb_ = std::move(cb);
	}
	void set_close_callback(EventCallback cb)
	{
		close_cb_ = std::move(cb);
	}
	void set_error_callback(EventCallback cb)
	{
		error_cb_ = std::move(cb);
	}
	
	Channel(EventLoop* loop, SelectableFD* sfd);
	~Channel();

	int fd() const;
	void set_revents(int revt) { revents_ = revt;}
	int revents() const { return revents_;}
	int events() const { return events_;}

	bool is_none_event() const { return events_ == kNoneEvent;}
	bool is_write_event() const { return events_ & kWriteEvent;}
	bool is_read_event() const { return events_ & kReadEvent;}
	
	void disable_all_event()
	{
		events_ = kNoneEvent;
		update();
	}
	void enable_read_event()
	{
		events_ |= kReadEvent;
		update();
	}
	void disable_read_event()
	{
		events_ &= ~kReadEvent;
		update();
	}
	void enable_write_event()
	{
		events_ |= kWriteEvent;
		update();
	}
	void disable_write_event()
	{
		events_ &= ~kWriteEvent;
		update();
	}

	// For Poller
	int index() { return index_;}
	void set_index(int idx) { index_ = idx;}

	// For EventLoop
	void handle_event(TimeStamp received_ms);
	EventLoop* owner_loop() { return owner_loop_;}
	
	// For debug
	std::string revents_to_string() const;
	std::string events_to_string() const;

	// *Not thread safe*, but run in own loop thread.
	void remove();
	void update();

private:
	static std::string events_to_string(int fd, int ev);

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

private:
	EventLoop* owner_loop_{nullptr};
	SelectableFD* select_fd_{nullptr};
	
	int	events_{0};
	int revents_{0};

	std::atomic<bool> added_to_loop_{false};
	std::atomic<bool> handling_event_{false};
	std::atomic<bool> logging_hup_{true};

	// update() event type, must have kNew == -1;
	int index_{-1};

	ReadEventCallback read_cb_;
	EventCallback write_cb_;
	EventCallback close_cb_;
	EventCallback error_cb_;

	DISALLOW_COPY_AND_ASSIGN(Channel);
};

}	// namespace annety

#endif	// 
