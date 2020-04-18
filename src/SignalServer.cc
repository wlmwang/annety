// By: wlmwang
// Date: Jun 17 2019

#include "SignalServer.h"
#include "Logging.h"
#include "EventLoop.h"
#include "Channel.h"
#include "SignalFD.h"
#include "threading/ThreadForward.h"

#if defined(OS_LINUX)
#include <sys/signalfd.h>  // signalfd_siginfo
#endif

#include <algorithm>

namespace annety
{
namespace {
// Similar to SIG_IGN macro, ignoring signal processing
// #define SIG_IGN ((__sighandler_t) 1)
void signal_ignore()
{
	DLOG(TRACE) << "annety::signal_ignore is calling";
}

}	// namespace anonymous

SignalServer::SignalServer(EventLoop* loop)
	: owner_loop_(loop)
	, signal_socket_(new SignalFD(true, true))
	, signal_channel_(new Channel(owner_loop_, signal_socket_.get()))
{
	// Only be called in the main thread.
	DCHECK(threads::is_main_thread());

	DLOG(TRACE) << "SignalServer::SignalServer" << " fd=" << 
		signal_socket_->internal_fd() << " is constructing";

	signal_channel_->set_read_callback(
		std::bind(&SignalServer::handle_read, this));
	signal_channel_->enable_read_event();
}

SignalServer::~SignalServer()
{
	owner_loop_->check_in_own_loop();

	DLOG(TRACE) << "SignalServer::~SignalServer" << " fd=" << 
		signal_socket_->internal_fd() << " is destructing";

	// Revert signal processor to default system handler
	revert_signal_in_own_loop();

	signal_channel_->disable_all_event();
	signal_channel_->remove();

	LOG_IF(ERROR, calling_signal_functor_) << "SignalServer::~SignalServer" 
		<< " fd=" << signal_socket_->internal_fd() << " is calling";
}

bool SignalServer::ismember_signal(int signo)
{
	owner_loop_->check_in_own_loop();

	return signals_.find(signo) != signals_.end();
}

void SignalServer::ignore_signal(int signo)
{
	add_signal(signo, std::bind(&signal_ignore));
}

void SignalServer::revert_signal()
{
	owner_loop_->run_in_own_loop(
		std::bind(&SignalServer::revert_signal_in_own_loop, this));
}

void SignalServer::add_signal(int signo, SignalCallback cb)
{
	owner_loop_->run_in_own_loop(
		std::bind(&SignalServer::add_signal_in_own_loop, this, signo, std::move(cb)));
}

void SignalServer::delete_signal(int signo)
{
	owner_loop_->run_in_own_loop(
		std::bind(&SignalServer::delete_signal_in_own_loop, this, signo));
}

void SignalServer::add_signal_in_own_loop(int signo, SignalCallback cb) 
{
	owner_loop_->check_in_own_loop();

	// Do not use insert(), because we may change the SignalCallback that 
	// has been added.
	signals_[signo] = std::move(cb);
	
	{
		SignalFD* sf = static_cast<SignalFD*>(signal_socket_.get());
		sf->signal_add(signo);
	}

	DLOG(TRACE) << "SignalServer::add_signal_in_own_loop signal " 
		<< signo << " success";
}

void SignalServer::delete_signal_in_own_loop(int signo) 
{
	owner_loop_->check_in_own_loop();

	SignalMap::iterator it = signals_.find(signo);
	if (it == signals_.end()) {
		return;
	}
	signals_.erase(it);

	{
		SignalFD* sf = static_cast<SignalFD*>(signal_socket_.get());
		sf->signal_delete(signo);
	}

	DLOG(TRACE) << "SignalServer::delete_signal_in_own_loop signal " 
		<< signo << " success";
}

void SignalServer::revert_signal_in_own_loop() 
{
	owner_loop_->check_in_own_loop();
	if (signals_.empty()) {
		return;
	}
	signals_.clear();

	{
		SignalFD* sf = static_cast<SignalFD*>(signal_socket_.get());
		sf->signal_revert();
	}

	DLOG(TRACE) << "SignalServer::revert_signal_in_own_loop signal success";
}

void SignalServer::handle_read()
{
	owner_loop_->check_in_own_loop();

	int signo = -1;
	TimeStamp curr = TimeStamp::now();

#if defined(OS_LINUX)
	{
		struct signalfd_siginfo siginfo;
		ssize_t n = signal_socket_->read(&siginfo, sizeof siginfo);
		if (n != sizeof siginfo) {
			PLOG(ERROR) << "SignalServer::handle_read reads " << n 
				<< " bytes instead of " << sizeof(siginfo);
			return;
		}
		signo = siginfo.ssi_signo;

		DLOG(TRACE) << "SignalServer::handle_read " << n << " at " << curr;
	}
#else
	{
		int64_t so;
		ssize_t n = signal_socket_->read(&so, sizeof so);
		if (n != sizeof so) {
			PLOG(ERROR) << "SignalServer::handle_read reads " << n 
				<< " bytes instead of " << sizeof(so);
			return;
		}
		signo = static_cast<int>(so);

		DLOG(TRACE) << "SignalServer::handle_read " << n << " at " << curr;
	}
#endif	// defined(OS_LINUX)

	// Callback the signal handler.
	if (signo != -1) {
		calling_signal_functor_ = true;
		auto it = signals_.find(signo);
		if (it != signals_.end()) {
		   (it->second)();
		}
		calling_signal_functor_ = false;
	}
}

}	// namespace annety
