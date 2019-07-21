// Refactoring: Anny Wang
// Date: Jul 15 2019

#include "SignalFD.h"
#include "Logging.h"

#if defined(OS_LINUX)
#include <sys/signalfd.h>  // signalfd
#else
#include "EventFD.h"
#endif

#include <unistd.h>
#include <signal.h>	// SIG_DFL, SIG_IGN

// \file <signal.h>
// /* Type of a signal handler. */
// typedef void (*__sighandler_t) (int);
//
// /* Structure describing the action to be taken when a signal arrives. */
// struct sigaction {
// 	__sighandler_t sa_handler;
// 	/* Additional set of signals to be blocked. */
// 	__sigset_t sa_mask;
// 	/* Special flags.  */
// 	int sa_flags;
// 	/* Restore handler.  */
// 	void (*sa_restorer) (void);
// };
//
// /* Get and/or set the action for signal SIG. */
// int sigaction(int __sig, const struct sigaction *__restrict __act,
// 	struct sigaction *__restrict __oact);
//

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

#if !defined(OS_LINUX)
namespace {
static_assert(sizeof(int64_t) >= sizeof(int), "loss of precision");

// no need to care about thread-safe, because only one SignalFD instance in main thread.
SignalFD* g_signal_fd{nullptr};
void signal_handler(int signo)
{
	if (g_signal_fd) {
		int64_t so = static_cast<int64_t>(signo);	// fixed length
		g_signal_fd->write(static_cast<void*>(&so), sizeof(int64_t));
	}
}
}	// namespace anonymous
#endif

SignalFD::SignalFD(bool nonblock, bool cloexec)
{
	// sigemptyset is macros at some platform
	PCHECK(sigemptyset(&sigset_) == 0);

#if defined(OS_LINUX)
	nonblock_ = nonblock;
	cloexec_ = cloexec;
	fd_ = internal::signalfd(&sigset_, nonblock_, cloexec_);
#else
	{
		CHECK(!g_signal_fd) << "SignalFD::SignalFD has been created at " << g_signal_fd;
		g_signal_fd = this;
	}
	ev_.reset(new EventFD(nonblock, cloexec));
	fd_ = ev_->internal_fd();
#endif

	PCHECK(fd_ >= 0);
	LOG(TRACE) << "SignalFD::SignalFD" << " fd=" << fd_ << " is constructing";
}

SignalFD::~SignalFD()
{
#if !defined(OS_LINUX)
	fd_ = -1;
	g_signal_fd = nullptr;
#endif
}

ssize_t SignalFD::read(void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::read(buf, len);
#else
	return ev_->read(buf, len);
#endif
}

ssize_t SignalFD::write(const void *buf, size_t len)
{
#if defined(OS_LINUX)
	return SelectableFD::write(buf, len);
#else
	return ev_->write(buf, len);
#endif
}

void SignalFD::signal_add(int signo)
{
	if (signal_ismember(signo) == 1) {
		return;
	}
	// sigaddset is macros at some platform
	PCHECK(sigaddset(&sigset_, signo) == 0);

#if defined(OS_LINUX)
	PCHECK(::sigprocmask(SIG_BLOCK, &sigset_, NULL) == 0);
	PCHECK(internal::signalfd(&sigset_, nonblock_, cloexec_, fd_) >= 0);
#else
	CHECK(signo_.find(signo) == signo_.end());

	struct sigaction act;
	act.sa_handler = &signal_handler;
	// SA_NODEFER:
	// 	the signal can still be received when calling the signal handler.
	// SA_RESTART:
	// 	restart the interrupted system calls.
	// SA_RESETHAND:
	// 	reset the signal's handler to SIG_DFL when calling the signal handler.
	act.sa_flags = SA_NODEFER | SA_RESTART;
	PCHECK(sigemptyset(&act.sa_mask) == 0);
	PCHECK(::sigaction(signo, &act, NULL) == 0);
	{
		auto ok = signo_.insert(signo);
		PCHECK(ok.second);
	}
#endif
}

void SignalFD::signal_delete(int signo)
{
	if (signal_ismember(signo) == 0) {
		return;
	}
	// sigdelset is macros at some platform
	PCHECK(sigdelset(&sigset_, signo) == 0);

#if defined(OS_LINUX)
	PCHECK(::sigprocmask(SIG_SETMASK, &sigset_, NULL) == 0);
	PCHECK(internal::signalfd(&sigset_, nonblock_, cloexec_, fd_) >= 0);
#else
	CHECK(signo_.find(signo) != signo_.end());
	
	struct sigaction act;
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	PCHECK(sigemptyset(&act.sa_mask) == 0);
	PCHECK(::sigaction(signo, &act, NULL) == 0);
	{
		size_t n = signo_.erase(signo);
		PCHECK(n == 1);
	}
#endif
}

void SignalFD::signal_revert()
{
	// sigisemptyset() is GNU platform's interface
	// FIXME: use signo_.empty()

	// sigemptyset is macros at some platform
	PCHECK(sigemptyset(&sigset_) == 0);

#if defined(OS_LINUX)
	PCHECK(::sigprocmask(SIG_SETMASK, &sigset_, NULL) == 0);
	PCHECK(internal::signalfd(&sigset_, nonblock_, cloexec_, fd_) >= 0);
#else
	if (signo_.empty()) {
		return;
	}
	// reset all signal handler to default
	struct sigaction act;
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	PCHECK(sigemptyset(&act.sa_mask) == 0);
	for (auto it = signo_.begin(); it != signo_.end(); it++) {
		PCHECK(::sigaction(*it, &act, NULL) == 0);
	}
	{
		signo_.clear();
	}
#endif
}

int SignalFD::signal_ismember(int signo)
{
	// sigismember is macros at some platform
	int rt = sigismember(&sigset_, signo);
	PCHECK(rt >= 0);
	return rt;
}

}	// namespace annety
