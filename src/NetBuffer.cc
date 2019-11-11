// By: wlmwang
// Date: Jun 22 2019

#include "NetBuffer.h"
#include "SocketsUtil.h"

#include <errno.h>

namespace annety
{
ssize_t NetBuffer::read_fd(int fd, int* err)
{
	const size_t writable = writable_bytes();

	char extrabuf[65536];
	struct iovec vec[2];
	vec[0].iov_base = begin_write();
	vec[0].iov_len = writable;
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;

	// when there is enough space in this buffer, don't read into extrabuf.
	// when extrabuf is used, we read 128k-1 bytes at most.
	const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;

	// Use vec[0] first, and then vec[1]
	const ssize_t n = sockets::readv(fd, vec, iovcnt);
	if (n < 0) {
		if (err != nullptr) *err = errno;
	} else if (static_cast<size_t>(n) <= writable) {
		has_written(n);
	} else {
		has_written(writable);
		append(extrabuf, n - writable);
	}
	// if (n == writable + sizeof extrabuf) {
	// 	goto line_12;
	// }
	return n;
}

}	// namespace annety
