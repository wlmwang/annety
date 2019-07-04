// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TimerFD.h"
#include "Logging.h"
#include "Time.h"

#if defined(OS_LINUX)
#include <sys/timerfd.h>  // timerfd
#endif

#include <unistd.h>

namespace annety
{
namespace internal
{
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
	return ::timerfd_create(CLOCK_MONOTONIC, flags);
}

int reset_timerfd(int timerfd, Time expired)
{
	// wake up loop by timerfd_settime()
	struct itimerspec it;
	::memset(&it, 0, sizeof it);

	TimeDelta delta = expired - Time::now();
	it.it_value.tv_sec = static_cast<time_t>(delta.in_seconds());
	it.it_value.tv_nsec = static_cast<long long>(delta.in_microseconds_f() * 1000);

	return ::timerfd_settime(timerfd, 0, &it, NULL);
}
#endif

}	// namespace internal

TimerFD::TimerFD(bool nonblock, bool cloexec)
{
#if defined(OS_LINUX)
	fd_ = internal::create_timerfd(true, true);
#else
	//...
#endif
	DPCHECK(fd_ >= 0);
	LOG(TRACE) << "TimerFD::TimerFD" << " of fd=" << fd_ << " is constructing";
}

int TimerFD::close()
{
#if defined(OS_LINUX)
	return SelectableFD::close();
#else
	//...
#endif
}

ssize_t TimerFD::read(void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::read(buf, len);
#else
	//...
#endif
}

ssize_t TimerFD::write(const void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::write(buf, len);
#else
	//...
#endif
}

}	// namespace annety
