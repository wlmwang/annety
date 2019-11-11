// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_SELECTABLE_FD_H_
#define ANT_SELECTABLE_FD_H_

#include "Macros.h"

#include <sys/types.h>

namespace annety
{
// a selectable I/O file descriptor
//
// this class owns life-time of the file description,
// the wrapped file descriptor may be a socket, eventfd, timerfd or signalfd
class SelectableFD
{
public:
	SelectableFD() = default;
	explicit SelectableFD(int fd);

	virtual ~SelectableFD();
	
	virtual ssize_t read(void *buf, size_t len);
	virtual ssize_t write(const void *buf, size_t len);

	int internal_fd() const { return fd_;}

protected:
	int fd_{-1};
	
	DISALLOW_COPY_AND_ASSIGN(SelectableFD);
};

}	// namespace annety

#endif	// ANT_SELECTABLE_FD_H_
