// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventFD.h"
#include "Logging.h"

#include <unistd.h>
#if !defined(OS_MACOSX)
#include <sys/eventfd.h>  // eventfd
#else
#include "SocketsUtil.h"
#endif

namespace annety
{
EventFD::EventFD(bool nonblock, bool cloexec)
{
#if !defined(OS_MACOSX)
	int flags = 0;
	if (nonblock) {
		flags |= EFD_NONBLOCK;
	}
	if (cloexec) {
		flags |= EFD_CLOEXEC;
	}

	fd_ = ::eventfd(0, flags);
#else
	// fd_ = sockets::socket(AF_UNIX, true, true);
#endif
	PLOG_IF(ERROR, fd_ < 0) << "::eventfd failed";
}

}	// namespace annety
