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
#include <signal.h>		// SIG_DFL, SIG_IGN

//
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

namespace
{
// ::sigaction bind handler
thread_local SignalFD* tls_signal_fd{nullptr};
void signal_handler(int signo)
{
	static_assert(sizeof(int64_t) >= sizeof(int), "loss of precision");
	if (tls_signal_fd) {
		int64_t so = static_cast<int64_t>(signo);	// fixed length
		tls_signal_fd->write(static_cast<void*>(&so), sizeof(int64_t));
	}
}
}	// namespace anonymous

SignalFD::SignalFD(bool nonblock, bool cloexec)
{
#if defined(OS_LINUX)
	nonblock_ = nonblock;
	cloexec_ = cloexec;
	PCHECK(::sigemptyset(&mask_) == 0);
	fd_ = internal::signalfd(&mask_, nonblock_, cloexec_);
#else
	tls_signal_fd = this;
	ev_.reset(new EventFD(nonblock, cloexec));
	fd_ = ev_->internal_fd();
#endif

	PCHECK(fd_ >= 0);
	LOG(TRACE) << "SignalFD::SignalFD" << " fd=" << fd_ << " is constructing";
}

SignalFD::~SignalFD()
{
#if !defined(OS_LINUX)
	tls_signal_fd = nullptr;
#endif
}

int SignalFD::close()
{
#if defined(OS_LINUX)
	return SelectableFD::close();
#else
	return ev_->close();
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
#if defined(OS_LINUX)
	PCHECK(::sigaddset(&mask_, signo) == 0);
	PCHECK(::sigprocmask(SIG_BLOCK, &mask_, NULL) == 0);
	PCHECK(internal::signalfd(&mask_, nonblock_, cloexec_, fd_) >= 0);
#else
	std::set<int>::iterator it = signo_.find(signo);
	if (it != signo_.end()) {
		return;
	}
	struct sigaction act;
	act.sa_handler = &signal_handler;
	// SA_NODEFER:
	// 	when executing the signal processing handler, the signal can still be received.
	// SA_RESTART:
	// 	restart interrupted system calls
	act.sa_flags = SA_NODEFER | SA_RESTART;
	PCHECK(sigemptyset(&act.sa_mask) == 0);
	PCHECK(::sigaction(signo, &act, NULL) == 0);
	{
		auto ok = signo_.insert(signo);
		PCHECK(ok.second);
	}
#endif
}

void SignalFD::signal_del(int signo)
{
#if defined(OS_LINUX)
	PCHECK(::sigdelset(&mask_, signo) == 0);
	PCHECK(::sigprocmask(SIG_BLOCK, &mask_, NULL) == 0);
	PCHECK(internal::signalfd(&mask_, nonblock_, cloexec_, fd_) >= 0);
#else
	std::set<int>::iterator it = signo_.find(signo);
	if (it != signo_.end()) {
		struct sigaction act;
		act.sa_handler = SIG_DFL;
		act.sa_flags = 0;
		PCHECK(sigemptyset(&act.sa_mask) == 0);
		PCHECK(::sigaction(signo, &act, NULL) == 0);
		{
			size_t n = signo_.erase(signo);
			PCHECK(n == 1);
		}
	}
#endif
}

void SignalFD::signal_reset()
{
#if defined(OS_LINUX)
	PCHECK(::sigemptyset(&mask_) == 0);
	PCHECK(::sigprocmask(SIG_BLOCK, &mask_, NULL) == 0);
	PCHECK(internal::signalfd(&mask_, nonblock_, cloexec_, fd_) >= 0);
#else
	// reset all signal to default
	if (!signo_.empty()) {
		struct sigaction act;
		act.sa_handler = SIG_DFL;
		act.sa_flags = 0;
		PCHECK(sigemptyset(&act.sa_mask) == 0);
		for (auto it = signo_.begin(); it != signo_.end(); it++) {
			PCHECK(::sigaction(*it, &act, NULL) == 0);
		}
	}
#endif
}

}	// namespace annety
