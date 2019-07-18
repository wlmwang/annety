// Refactoring: Anny Wang
// Date: Jul 15 2019

#ifndef ANT_SIGNAL_FD_H_
#define ANT_SIGNAL_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

#if !defined(OS_LINUX)
#include <set>
#include <memory>
#endif

#include <signal.h>	// for signal macros

namespace annety
{
class SignalFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	SignalFD() = delete;
	SignalFD(bool nonblock, bool cloexec);
	virtual ~SignalFD() override;

	virtual int close() override;
	virtual ssize_t read(void *buf, size_t len) override;
	virtual ssize_t write(const void *buf, size_t len) override;

	void signal_add(int signo);
	void signal_del(int signo);
	void signal_reset();
	// void signal_set(int signo);
	// int signal_ismember(int signo);

private:
#if defined(OS_LINUX)
	bool nonblock_;
	bool cloexec_;
	sigset_t mask_;
#else
	std::set<int> signo_;
	std::unique_ptr<SelectableFD> ev_;
#endif

	DISALLOW_COPY_AND_ASSIGN(SignalFD);
};

}	// namespace annety

#endif	// ANT_SIGNAL_FD_H_
