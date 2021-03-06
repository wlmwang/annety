// By: wlmwang
// Date: Jun 17 2019

#include "TimerFD.h"
#include "Logging.h"

#if defined(OS_LINUX)
#include <sys/timerfd.h>  // timerfd,itimerspec
#else
#include "EventFD.h"
#endif

#include <unistd.h>		// ::memset

namespace annety
{
namespace internal {
#if defined(OS_LINUX)
int create_timerfd(bool nonblock, bool cloexec)
{
	int flags = 0;
	if (nonblock) {
		flags |= TFD_NONBLOCK;
	}
	if (cloexec) {
		flags |= TFD_CLOEXEC;
	}

	// CLOCK_REALTIME
	// A settable system-wide clock (manual changes the system time will 
	// affect its continuity).
	//
	// CLOCK_MONOTONIC
	// A nonsettable clock that is not affected by discontinuous changes 
	// in the system clock (e.g., manual changes to system time).
	return ::timerfd_create(CLOCK_MONOTONIC, flags);
}

int reset_timerfd(int timerfd, TimeDelta delta_ms)
{
	if (delta_ms.is_null()) {
		return -1;
	}

	// In Linux versions up to and including 2.6.26, flags must 
	// be specified as zero.
	struct itimerspec it;
	::memset(&it, 0, sizeof it);

	// We only use the one-time wakeup of timerfd, and the repeat timer will be 
	// reset when the timer timeout.
	it.it_value = delta_ms.to_timespec();

	// `it_value` is a offset value from current time when the second param is 0
	return ::timerfd_settime(timerfd, 0, &it, NULL);
}
#endif

}	// namespace internal

TimerFD::TimerFD(bool nonblock, bool cloexec)
{
#if defined(OS_LINUX)
	fd_ = internal::create_timerfd(nonblock, cloexec);
#else
	ev_.reset(new EventFD(nonblock, cloexec));
	fd_ = ev_->internal_fd();
#endif
	PCHECK(fd_ >= 0);
	DLOG(TRACE) << "TimerFD::TimerFD" << " fd=" << fd_ << " is constructing";
}

TimerFD::~TimerFD()
{
#if !defined(OS_LINUX)
	// fd_ lifetime is controlled by the ev_ (EventFD).
	fd_ = -1;
#endif
}

ssize_t TimerFD::read(void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::read(buf, len);
#else
	return ev_->read(buf, len);
#endif
}

ssize_t TimerFD::write(const void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::write(buf, len);
#else
	return ev_->write(buf, len);
#endif
}

void TimerFD::reset(const TimeDelta& delta_ms)
{
#if defined(OS_LINUX)
	int rt = internal::reset_timerfd(fd_, delta_ms);
	PCHECK(rt >= 0);
#else
	// In TimerPool class to set_poll_timeout()
#endif
}

}	// namespace annety
