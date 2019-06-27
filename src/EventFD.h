// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_FD_H_
#define ANT_EVENT_FD_H_

#include "Macros.h"
#include "SelectableFD.h"

namespace annety
{
class EventFD : public SelectableFD
{
public:
	using SelectableFD::SelectableFD;

	EventFD() = delete;
	EventFD(bool nonblock, bool cloexec);

	virtual int close() override;
	virtual ssize_t read(void *buf, size_t len) override;
	virtual ssize_t write(const void *buf, size_t len) override;

private:
#if !defined(OS_LINUX)
	int fds_[2];
#endif

	DISALLOW_COPY_AND_ASSIGN(EventFD);
};

}	// namespace annety

#endif	// _ANT_EVENT_FD_H_
