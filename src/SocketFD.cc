// Refactoring: Anny Wang
// Date: Jun 17 2019

#include "SocketFD.h"
#include "SocketsUtil.h"
#include "Logging.h"

namespace annety
{
SocketFD::SocketFD(bool nonblock, bool cloexec)
{
	fd_ = sockets::socket(AF_INET, nonblock, cloexec);
	DCHECK(fd_ >= 0);
}

SocketFD::~SocketFD() = default;

}	// namespace annety
