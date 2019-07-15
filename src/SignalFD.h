// Refactoring: Anny Wang
// Date: Jul 15 2019

#ifndef ANT_SIGNAL_FD_H_
#define ANT_SIGNAL_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

#include <signal.h>

namespace annety
{
class SignalFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	SignalFD() = delete;
	SignalFD(bool nonblock, bool cloexec);

	virtual int close() override;
	virtual ssize_t read(void *buf, size_t len) override;
	virtual ssize_t write(const void *buf, size_t len) override;

	void signal_add(int signo);
	// void signal_del(int signo);
	// bool signal_ismember(int signo);

private:
	bool nonblock_;
	bool cloexec_;
	sigset_t mask_;

	DISALLOW_COPY_AND_ASSIGN(SignalFD);
};

}	// namespace annety

#endif	// ANT_SIGNAL_FD_H_
