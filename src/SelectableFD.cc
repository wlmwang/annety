// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SelectableFD.h"
#include "SocketsUtil.h"

namespace annety
{
int SelectableFD::close()
{
	return sockets::close(fd_);
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
