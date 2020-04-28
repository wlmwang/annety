// By: wlmwang
// Date: Jun 17 2019

#include "SocketFD.h"
#include "SocketsUtil.h"
#include "Logging.h"

namespace annety
{
SocketFD::SocketFD(sa_family_t family, bool nonblock, bool cloexec)
{
	fd_ = sockets::socket(family, nonblock, cloexec);
	CHECK(fd_ >= 0);
}

}	// namespace annety
