# Introduction
	Annety is a multi-threaded reactor pattern network library based on C++11.
	Annety uses many codes modified from open source projects (chromium, muduo, 
	nginx, leveldb, etc).

# Requires
    GCC >= 4.8 or Clang >= 3.3
    Linux >= 2.6.28 (for epoll/timerfd/eventfd)
    POSIX(Darwin/FreeBSD/...) (for development)
    cmake >= 2.8.5

# Tested
	MacOS 10.12
	Unbuntu 14.04
	CentOS 7
	Debian 7

# Build
	BUILD_TYPE=release ./build.sh

# Document
	Annety took a week alone, adding a lot of comments to the source code.
* [chinese detail](document/README.md)

# Thanks
	Annety uses many codes modified from open source projects (chromium, muduo, 
	nginx, leveldb, etc). Yes, At the beginning of creating this library, one of 
	my hard requirements was to refer to some excellent open source projects. 
	For this hard requirement, the development of the library is slow, but it is 
	worth it. Standing on the shoulders of giants, make the code is very reliable.
	
	Annety's cross-platform reference chromium; thread model reference muduo; 
	checksum algorithm reference nginx; although the wakeup mechanism is new(pipe(2) 
	call), but it refers to nginx and muduo (For this kind of "hybrid" utility, 
	there are many in annety. such as threading, timer, LOG/CHECK, etc).
	THANKS TO ALL OF THEM.
