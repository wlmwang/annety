// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SignalServer.h"
#include "Logging.h"
#include "EventLoop.h"
#include "Channel.h"
#include "SignalFD.h"

#if defined(OS_LINUX)
#include <sys/signalfd.h>  // signalfd_siginfo
#endif

#include <algorithm>

namespace annety
{
SignalServer::SignalServer(EventLoop* loop)
	: owner_loop_(loop)
	, signal_socket_(new SignalFD(true, true))
	, signal_channel_(new Channel(owner_loop_, signal_socket_.get()))
{
	LOG(TRACE) << "SignalServer::SignalServer" << " fd=" << 
		signal_socket_->internal_fd() << " is constructing";

	signal_channel_->set_read_callback(
		std::bind(&SignalServer::handle_read, this));

	signal_channel_->enable_read_event();
}

SignalServer::~SignalServer()
{
	LOG(TRACE) << "SignalServer::~SignalServer" << " fd=" << 
		signal_socket_->internal_fd() << " is destructing";
}

void SignalServer::add_signal(int signo, SignalCallback cb)
{
	owner_loop_->run_in_own_loop(
		std::bind(&SignalServer::add_signal_in_own_loop, this, signo, std::move(cb)));
}

void SignalServer::add_signal_in_own_loop(int signo, SignalCallback cb) 
{
	owner_loop_->check_in_own_loop();

	SignalFD* sf = static_cast<SignalFD*>(signal_socket_.get());
	sf->signal_add(signo);

	signals_[signo] = std::move(cb);

	LOG(TRACE) << "SignalServer::add_signal_in_own_loop add signal " 
		<< signo << " success";
}

void SignalServer::handle_read()
{
	owner_loop_->check_in_own_loop();

	struct signalfd_siginfo siginfo;
	Time curr = Time::now();
	{
		ssize_t n = signal_socket_->read(&siginfo, sizeof siginfo);
		if (n != sizeof siginfo) {
			PLOG(ERROR) << "SignalServer::handle_read reads " << n << " bytes instead of " << sizeof(siginfo);
			return;
		}
		LOG(TRACE) << "SignalServer::handle_read " << n << " at " << curr;
	}

	{
		auto it = signals_.find(siginfo.ssi_signo);
		if (it != signals_.end()) {
		   (it->second)();
		}
	}
}

}	// namespace annety
