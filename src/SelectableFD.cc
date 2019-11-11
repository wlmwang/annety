// By: wlmwang
// Date: Jun 17 2019

#include "SelectableFD.h"
#include "SocketsUtil.h"
#include "Logging.h"

namespace annety
{
SelectableFD::SelectableFD(int fd) : fd_(fd)
{
	LOG_IF(ERROR, fd_ < 0) << "fd invaild " << fd_;
}

SelectableFD::~SelectableFD()
{
	LOG(TRACE) << "SelectableFD::~SelectableFD closing fd=" << fd_;
	if (fd_ != -1) {
		sockets::close(fd_);
	}
}

ssize_t SelectableFD::read(void *buf, size_t len)
{
	return sockets::read(fd_, buf, len);
}

ssize_t SelectableFD::write(const void *buf, size_t len)
{
	return sockets::write(fd_, buf, len);
}

}	// namespace annety
