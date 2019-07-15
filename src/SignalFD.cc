// Refactoring: Anny Wang
// Date: Jul 15 2019

#include "SignalFD.h"
#include "Logging.h"

#if defined(OS_LINUX)
#include <signal.h>
#include <sys/signalfd.h>  // signalfd
#else
// #include "EventFD.h"
#endif

#include <unistd.h>

namespace annety
{
namespace internal
{
#if defined(OS_LINUX)
int signalfd(const sigset_t* mask, bool nonblock, bool cloexec, int fd = -1)
{
	int flags = 0;
	if (nonblock) {
		flags |= SFD_NONBLOCK;
	}
	if (cloexec) {
		flags |= SFD_CLOEXEC;
	}
	return ::signalfd(fd, mask, flags);
}
#endif

}	// namespace internal

SignalFD::SignalFD(bool nonblock, bool cloexec)
	: nonblock_(nonblock)
	, cloexec_(cloexec)
{
#if defined(OS_LINUX)
	::sigemptyset(&mask_);
	fd_ = internal::signalfd(&mask_, nonblock_, cloexec_);
#else
	// ev_.reset(new EventFD(nonblock, cloexec));
	// fd_ = ev_->internal_fd();
#endif
	PCHECK(fd_ >= 0);
	LOG(TRACE) << "SignalFD::SignalFD" << " fd=" << fd_ << " is constructing";
}

int SignalFD::close()
{
#if defined(OS_LINUX)
	return SelectableFD::close();
#else
	//return ev_->close();
#endif
}

ssize_t SignalFD::read(void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::read(buf, len);
#else
	//return ev_->read(buf, len);
#endif
}

ssize_t SignalFD::write(const void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::write(buf, len);
#else
	//return ev_->write(buf, len);
#endif
}

void SignalFD::signal_add(int signo)
{
#if defined(OS_LINUX)
	PCHECK(::sigaddset(&mask_, signo) == 0);
	PCHECK(::sigprocmask(SIG_BLOCK, &mask_, NULL) == 0);
	PCHECK(internal::signalfd(&mask_, nonblock_, cloexec_, fd_) >= 0);
#else
	// ...
#endif
}

// void SignalFD::signal_del(int signo)
// {
// 	//...
// }

}	// namespace annety
