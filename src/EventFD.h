// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_EVENT_FD_H_
#define ANT_EVENT_FD_H_

#include "Macros.h"

namespace annety {
class EventFD {
public:
	explicit EventFD(bool nonblock = true, bool cloexec = true);
	explicit EventFD(int evtfd) : fd_(evtfd) {}

	~EventFD();

	int internal_fd() const {
		return fd_;
	}

private:
	int fd_;

	DISALLOW_COPY_AND_ASSIGN(EventFD);
};

}	// namespace annety

#endif	// _ANT_EVENT_FD_H_
