// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventFD.h"
#include "Logging.h"

#include <unistd.h>
#include <sys/eventfd.h>  // eventfd

namespace annety {
EventFD::EventFD(bool nonblock, bool cloexec) {
	int flags = 0;
	if (nonblock) {
		flags |= EFD_NONBLOCK;
	}
	if (cloexec) {
		flags |= EFD_CLOEXEC;
	}

	fd_ = ::eventfd(0, flags);
	PLOG_IF(ERROR, fd_ < 0) << "::eventfd failed";
}

EventFD::~EventFD() {
	int ret = ::close(fd_);
	PLOG_IF(ERROR, ret < 0) << "::close failed";
}

}	// namespace annety
