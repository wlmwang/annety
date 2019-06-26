// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_SELECTABLE_FD_H_
#define ANT_SELECTABLE_FD_H_

#include "Macros.h"
#include "Logging.h"

#include <sys/socket.h>	// sa_family_t,iovec

namespace annety
{
// Wrapper for selectable the file descriptor,
// and the Wrapper holds life-time of the file description
//
// The file descriptor could be a socket, eventfd, timerfd or signalfd
class SelectableFD
{
public:
	SelectableFD() = default;
	explicit SelectableFD(int fd) : fd_(fd)
	{
		LOG_IF(ERROR, fd_ < 0) << "fd invaild " << fd_;
	}

	virtual ~SelectableFD()
	{
		LOG(TRACE) << "closing fd " << fd_;
		close();
	}

	virtual int close();
	virtual ssize_t read(void *buf, size_t len);
	virtual ssize_t write(const void *buf, size_t len);

	int internal_fd() const { return fd_;}

protected:
	int fd_{-1};
	
	DISALLOW_COPY_AND_ASSIGN(SelectableFD);
};

}	// namespace annety

#endif	// ANT_SELECTABLE_FD_H_
