// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_SELECTABLE_FD_H_
#define ANT_SELECTABLE_FD_H_

#include "Macros.h"
#include "Logging.h"

namespace annety
{
class SelectableFD
{
public:
	SelectableFD() = default;
	
	explicit SelectableFD(int fd) : fd_(fd)
	{
		LOG_IF(ERROR, fd_ < 0) << "fd invaild " << fd_;
	}

	virtual ~SelectableFD();
	
	int internal_fd() const { return fd_;}

protected:
	int fd_{-1};
	
	DISALLOW_COPY_AND_ASSIGN(SelectableFD);
};

}	// namespace annety

#endif	// ANT_SELECTABLE_FD_H_