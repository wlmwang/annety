// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "EventFD.h"
#include "Logging.h"

#if defined(OS_LINUX)
#include <sys/eventfd.h>  // eventfd
#else
#include "SocketsUtil.h"
#include "ScopedFile.h"
#include <fcntl.h>
#endif

namespace annety
{
namespace internal
{
#if defined(OS_LINUX)
int eventfd(bool nonblock, bool cloexec)
{
	int flags = 0;
	if (nonblock) {
		flags |= EFD_NONBLOCK;
	}
	if (cloexec) {
		flags |= EFD_CLOEXEC;
	}
	return ::eventfd(0, flags);
}
#else
bool pipe(int fds[2], bool nonblock, bool cloexec)
{
	int raw_fds[2];
	if (::pipe(raw_fds) != 0) {
		return false;
	}

	ScopedFD fd_out(raw_fds[0]);
	ScopedFD fd_in(raw_fds[1]);
	if (cloexec && !sockets::set_close_on_exec(fd_out.get())) {
		return false;
	}
	if (cloexec && !sockets::set_close_on_exec(fd_in.get())) {
		return false;
	}
	if (nonblock && !sockets::set_non_blocking(fd_out.get())) {
		return false;
	}
	if (nonblock && !sockets::set_non_blocking(fd_in.get())) {
		return false;
	}
	fds[0] = fd_out.release();
	fds[1] = fd_in.release();
	return true;
}
#endif

}	// namespace internal

EventFD::EventFD(bool nonblock, bool cloexec)
{
#if defined(OS_LINUX)
	fd_ = internal::eventfd(true, true);
#else
	DCHECK(internal::pipe(fds_, true, true));
	fd_ = fds_[0];	// for channel register readable event
#endif
	DPCHECK(fd_ >= 0);
}

int EventFD::close()
{
#if defined(OS_LINUX)
	return SelectableFD::close();
#else
	int r1 = sockets::close(fds_[0]);
	int r2 = sockets::close(fds_[1]);
	return r1 != 0? r1: r2;
#endif
}

ssize_t EventFD::read(void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::read(buf, len);
#else
	return sockets::read(fds_[0], buf, len);
#endif
}

ssize_t EventFD::write(const void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::write(buf, len);
#else
	return sockets::write(fds_[1], buf, len);
#endif
}

}	// namespace annety
