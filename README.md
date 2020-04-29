# Introduction
	Annety is a multi-threaded reactor pattern network library based on C++11.

# Requires (POSIX && C++11)
    GCC >= 4.8 or Clang >= 3.3
    Linux >= 2.6.28 (for epoll/timerfd/eventfd)
    POSIX(Darwin/...) (for development)
    cmake >= 2.8.5

# Tested
	macOS 10.12
	Unbuntu 14.04
	CentOS 7
	Debian 7

# Build && Install
	./build.sh

# Document
	Annety took a week alone, adding a lot of comments to the source code.
	// todo.

# Addition
	Annety uses many codes modified from open source projects (chromium, muduo, 
	nginx, leveldb, etc). Yes, At the beginning of creating the library, one of 
	my hard requirements was to refer to some excellent open source projects. 
	For this hard requirement, the development of the library is slow, but it is 
	worth it. Standing on the shoulders of giants, make the code is very reliable.
	THANKS FOR ALL OF THEM.
	Annety's cross-platform reference chromium; thread model reference muduo; 
	checksum algorithm reference nginx; although the wakeup mechanism is new(
	pipe(2) call), but it refers to nginx and muduo (For this kind of "mixed", 
	there are many in annety, such as threading, LOG/CHECK, etc).
	Annety performance is very high, it can easily run full 1Gbps (10Gbps is ok).
