

#ifndef _ANT_BUILD_CONFIG_H_
#define _ANT_BUILD_CONFIG_H_

// A set of macros to use for platform detection.
#if defined(__APPLE__)
#include <TargetConditionals.h>
#define OS_MACOSX 1
#elif defined(__linux__)
#define OS_LINUX 1
#elif defined(__FreeBSD__)
#define OS_FREEBSD 1
#elif defined(__NetBSD__)
#define OS_NETBSD 1
#elif defined(__OpenBSD__)
#define OS_OPENBSD 1
#elif defined(__sun)
#define OS_SOLARIS 1
#else
#error Please add support for your platform in build_config.h
#endif

// For access to standard BSD features, use OS_BSD instead of a
// more specific macro.
#if defined(OS_FREEBSD) || defined(OS_NETBSD) || defined(OS_OPENBSD)
#define OS_BSD 1
#endif

// For access to standard POSIXish features, use OS_POSIX instead of a
// more specific macro.
#if defined(OS_FREEBSD) || defined(OS_LINUX) || defined(OS_MACOSX) || \
    defined(OS_SOLARIS) || defined(OS_NETBSD) || defined(OS_OPENBSD) ||  \
#define OS_POSIX 1
#endif

// Use tcmalloc
#if defined(OS_LINUX) && !defined(NO_TCMALLOC)
#define USE_TCMALLOC 1
#endif

// Compiler detection for cxx0x
#if defined(__GNUC__) && !(defined(__GXX_EXPERIMENTAL_CXX0X__) && \
    __cplusplus >= 201103)
#error Do not support your compiler in build_config.h
#endif

#endif  // _ANT_BUILD_CONFIG_H_

