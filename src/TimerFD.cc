// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "TimerFD.h"
#include "Logging.h"

#include <unistd.h>
#include <sys/timerfd.h>  // timerfd

namespace annety {
TimerFD::TimerFD(bool nonblock, bool cloexec) {
	int flags = 0;
	if (nonblock) {
		flags |= TFD_NONBLOCK;
	}
	if (cloexec) {
		flags |= TFD_CLOEXEC;
	}

	fd_ = ::timerfd_create(CLOCK_MONOTONIC, flags);
	PLOG_IF(ERROR, fd_ < 0) << "::timerfd_create failed";
}

TimerFD::~TimerFD() {
	int ret = ::close(fd_);
	PLOG_IF(ERROR, ret < 0) << "::close failed";
}

}	// namespace annety
