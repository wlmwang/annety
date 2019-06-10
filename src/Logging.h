// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#ifndef ANT_LOGGING_H
#define ANT_LOGGING_H

#include "BuildConfig.h"		// ARCH_CPU_X86_FAMILY
#include "CompilerSpecific.h"	// UNLIKELY
#include "Macros.h"
#include "LogStream.h"
#include "Time.h"
#include "ScopedClearLastError.h"

#include <sstream>
#include <string>
#include <functional>
#include <utility>
#include <string.h>
#include <errno.h>	// errno

// Instructions
// ----------------------------------------------------------------------
//
// Make a bunch of macros for logging. The way to log things is to stream
// things to LOG(<a particular severity level>).  E.g.,
//
//   LOG(INFO) << "Found " << num_cookies << " cookies";
//
// You can also do conditional logging:
//
//   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
//
// There are also "debug mode" logging macros like the ones above:
//
//   DLOG(INFO) << "Found cookies";
//
//   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// Lastly, there is:
//
//   PLOG(ERROR) << "Couldn't do foo";
//   DPLOG(ERROR) << "Couldn't do foo";
//   PLOG_IF(ERROR, cond) << "Couldn't do foo";
//   DPLOG_IF(ERROR, cond) << "Couldn't do foo";
//   PCHECK(condition) << "Couldn't do foo";
//   DPCHECK(condition) << "Couldn't do foo";
//
// which append the last system error to the message in string form (taken from
// GetLastError() errno on POSIX).

// *All "debug mode" logging is compiled away to nothing for non-debug mode
// compiles.
// LOG_IF and development flags also work well together. because the code 
// can be compiled away sometimes.

// *The CHECK(condition) macro is active in both debug and release builds, 
// and effectively performs a LOG(FATAL) which terminates the process and
// generates a crashdump.

// *Very important*: logging a message at the FATAL severity level causes
// the program to terminate (after the message is logged).
// There is the special severity of DFATAL, which logs FATAL in debug mode,
// ERROR in normal mode.

namespace annety {
typedef int LogSeverity;
const LogSeverity LOG_VERBOSE = -1;  // This is level 1 verbosity
const LogSeverity LOG_INFO = 0;
const LogSeverity LOG_WARNING = 1;
const LogSeverity LOG_ERROR = 2;
const LogSeverity LOG_FATAL = 3;
const LogSeverity LOG_NUM_SEVERITIES = 4;

// LOG_DFATAL is LOG_FATAL in debug mode, ERROR in normal mode
#if defined(NDEBUG)
const LogSeverity LOG_DFATAL = LOG_ERROR;
#else
const LogSeverity LOG_DFATAL = LOG_FATAL;
#endif

// Do not use std::functional<>
using LogOutputHandlerFunction = void(*)(const char* msg, int len);
using LogFFlushHandlerFunction = void(*)();

LogOutputHandlerFunction SetLogOutputHandler(LogOutputHandlerFunction handler);
LogFFlushHandlerFunction SetLogFFlushHandler(LogFFlushHandlerFunction handler);

void SetMinLogLevel(int level);
int GetMinLogLevel();

// Used by LOG_IS_ON to lazy-evaluate stream arguments.
bool ShouldCreateLogMessage(int severity);

class LogMessage {
public:
	// todo
	// compile time calculation of basename of source file
	class Filename {
	public:
		template<int N>
		Filename(const char (&path)[N]) : basename_(path), size_(N-1) {
			const char* slash = ::strrchr(basename_, '/');
			if (slash) {
				basename_ = slash + 1;
				size_ -= basename_ - path;
			}
		}
		explicit Filename(const char* path) : basename_(path) {
			const char* slash = ::strrchr(path, '/');
			if (slash) {
				basename_ = slash + 1;
			}
			size_ = ::strlen(basename_);
		}

		const char* basename_;
		size_t size_;
	};

	// Used for {,D}LOG
	LogMessage(int line, const Filename& file, LogSeverity sev);

	// Used for {,D,P,DP}LOG
	LogMessage(int line, const Filename& file, LogSeverity sev, int err);

	// Used for {,D}CHECK
	LogMessage(int line, const Filename& file, LogSeverity sev, const std::string& msg);

	// Used for {,D,P,DP}CHECK
	LogMessage(int line, const Filename& file, LogSeverity sev, const std::string& msg, int err);

	~LogMessage();

	LogStream& stream() {
		return impl_.stream_;
	}

	LogSeverity severity() {
		return impl_.severity_;
	}

private:
	class Impl {
	public:
		Impl(int line, const Filename& file, LogSeverity sev, int err = 0);
		Impl(int line, const Filename& file, LogSeverity sev, const std::string& msg, 
			int err = 0);
		
		void begin();
		void endl();

	public:
		Time time_{Time::now()};
		LogStream stream_{};

		int line_;
		Filename file_;
		LogSeverity severity_;

		int errno_;
		ScopedClearLastError last_error_;
	};

private:
	Impl impl_;

	DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
public:
	LogMessageVoidify() = default;
	// This has to be an operator with a precedence lower than << but
	// higher than ?:
	void operator&(annety::LogStream&) {}
};

// errno on Linux
#define GET_LAST_ERRNO() (errno)

// A few definitions of macros that don't generate much code. These are used
// by LOG() and LOG_IF, etc. Since these are used all over our code, it's
// better to have compact code for these operations.
#define COMPACK_LOG_HANDLE(severity, ...)		\
	annety::LogMessage(__LINE__,				\
					   __FILE__,				\
					   annety::LOG_ ## severity,\
					   ##__VA_ARGS__)

#define LOG_IS_ON(severity) \
	(annety::ShouldCreateLogMessage(annety::LOG_ ## severity))

#define LOG_STREAM(severity, ...)	\
	COMPACK_LOG_HANDLE(severity, ##__VA_ARGS__).stream()

#define PLOG_STREAM(severity, ...)	\
	COMPACK_LOG_HANDLE(severity, ##__VA_ARGS__).stream()

#define LAZY_STREAM(stream, condition) \
	!(condition) ? (void)0 : annety::LogMessageVoidify() & (stream)

#define LOG(severity) \
	LAZY_STREAM( \
		LOG_STREAM(severity), LOG_IS_ON(severity) \
	)

#define LOG_IF(severity, condition) \
	LAZY_STREAM( \
		LOG_STREAM(severity), LOG_IS_ON(severity) && (condition) \
	)

#define PLOG(severity) \
	LAZY_STREAM( \
		PLOG_STREAM(severity, GET_LAST_ERRNO()), LOG_IS_ON(severity) \
	)

#define PLOG_IF(severity, condition) \
	LAZY_STREAM( \
		PLOG_STREAM(severity, GET_LAST_ERRNO()), LOG_IS_ON(severity) && (condition) \
	)

// {,D}CHECK* / DLOG*------------------------------------------------------------

// This is never instantiated, it's just used for EAT_STREAM_PARAMETERS to have
// an object of the correct type on the LHS of the unused part of the ternary
// operator.
extern annety::LogStream* g_swallow_stream;

// Note that g_swallow_stream is used instead of an arbitrary LOG() stream to
// avoid the creation of an object with a non-trivial destructor (LogMessage).
#define EAT_STREAM_PARAMETERS \
	true ? (void)0 : annety::LogMessageVoidify() & (*g_swallow_stream)

// Captures the result of a CHECK_EQ (for example) and facilitates testing as a
// boolean.
class CheckOpResult {
public:
	// |message| must be not-empty if and only if the check failed.
	// *Note: Move Construct will be called in CHECK_OP() 
	CheckOpResult(const std::string& message) : message_(message) {}
	CheckOpResult(std::string&& message) : message_(std::move(message)) {}

	// Returns true if the check succeeded.
	operator bool() const {
		return message_.empty();
	}
	// Returns the message.
	std::string message() {
		return message_;
	}

private:
	std::string message_;
};

// Crashes in the fastest possible way with no attempt at logging.
// There are different constraints to satisfy here, see http://crbug.com/664209
// for more context:
// - The trap instructions, and hence the PC value at crash time, have to be
//   distinct and not get folded into the same opcode by the compiler.
//   On Linux/Android this is tricky because GCC still folds identical
//   asm volatile blocks. The workaround is generating distinct opcodes for
//   each CHECK using the __COUNTER__ macro.
// - The debug info for the trap instruction has to be attributed to the source
//   line that has the CHECK(), to make crash reports actionable. This rules
//   out the ability of using a inline function, at least as long as clang
//   doesn't support attribute(artificial).
// - Failed CHECKs should produce a signal that is distinguishable from an
//   invalid memory access, to improve the actionability of crash reports.
// - The compiler should treat the CHECK as no-return instructions, so that the
//   trap code can be efficiently packed in the prologue of the function and
//   doesn't interfere with the main execution flow.
// - When debugging, developers shouldn't be able to accidentally step over a
//   CHECK. This is achieved by putting opcodes that will cause a non
//   continuable exception after the actual trap instruction.
// - Don't cause too much binary bloat.
#if defined(ARCH_CPU_X86_FAMILY)
// int 3 will generate a SIGTRAP.
#define TRAP_SEQUENCE()	\
	asm volatile(		\
		"int3; ud2; push %0;" ::"i"(static_cast<unsigned char>(__COUNTER__)))
#else	// !(ARCH_CPU_X86_FAMILY)
// Crash report accuracy will not be guaranteed on other architectures, but at
// least this will crash as expected.
#define TRAP_SEQUENCE() __builtin_trap()
#endif	// ARCH_CPU_X86_FAMILY

// CHECK() and the trap sequence can be invoked from a constexpr function.
// This could make compilation fail on GCC, as it forbids directly using inline
// asm inside a constexpr function. However, it allows calling a lambda
// expression including the same asm.
// The side effect is that the top of the stacktrace will not point to the
// calling function, but to this anonymous lambda. This is still useful as the
// full name of the lambda will typically include the name of the function that
// calls CHECK() and the debugger will still break at the right line of code.
#if !defined(__clang__)
#define WRAPPED_TRAP_SEQUENCE()		\
	do {							\
		[] { TRAP_SEQUENCE(); }();	\
	} while (false)
#else	// defined(__clang__)
#define WRAPPED_TRAP_SEQUENCE() TRAP_SEQUENCE()
#endif  // __clang__

#define IMMEDIATE_CRASH()		\
	({							\
		WRAPPED_TRAP_SEQUENCE();\
		__builtin_unreachable();\
	})

// The ANALYZER_ASSUME_TRUE(bool arg) macro adds compiler-specific hints
// to Clang which control what code paths are statically analyzed,
// and is meant to be used in conjunction with assert & assert-like functions.
// The expression is passed straight through if analysis isn't enabled.
//
// ANALYZER_SKIP_THIS_PATH() suppresses static analysis for the current
// codepath and any other branching codepaths that might follow.
#define ANALYZER_ASSUME_TRUE(arg) (arg)
#define ANALYZER_SKIP_THIS_PATH()
#define ANALYZER_ALLOW_UNUSED(var) static_cast<void>(var);

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.
//
// We make sure CHECK et al. always evaluates their arguments, as
// doing CHECK(FunctionWithSideEffect()) is a common idiom.
#if defined(NDEBUG)

// Make all CHECK functions discard their log strings to reduce code bloat, and
// improve performance, for official release builds.
//
// This is not calling BreakDebugger since this is called frequently, and
// calling an out-of-line function instead of a noreturn inline macro prevents
// compiler optimizations.
#define CHECK(condition) \
	UNLIKELY(!(condition)) ? IMMEDIATE_CRASH() : EAT_STREAM_PARAMETERS

// PCHECK includes the system error code, which is useful for determining
// why the condition failed. In official builds, preserve only the error code
// message so that it is available in crash reports. The stringified
// condition and any additional stream parameters are dropped.
#define PCHECK(condition)										\
	LAZY_STREAM(PLOG_STREAM(FATAL), UNLIKELY(!(condition)));	\
		EAT_STREAM_PARAMETERS

#define CHECK_OP(name, op, val1, val2) CHECK((val1) op (val2))

#else  // !(NDEBUG)

// Do as much work as possible out of line to reduce inline code size.
#define CHECK(condition)						\
	LAZY_STREAM(LOG_STREAM(FATAL, #condition), 	\
		!ANALYZER_ASSUME_TRUE(condition))
#define PCHECK(condition)											\
	LAZY_STREAM(PLOG_STREAM(FATAL, #condition, GET_LAST_ERRNO()),	\
		!ANALYZER_ASSUME_TRUE(condition))

// Helper macro for binary operators.
// Don't use this macro directly in your code, use CHECK_EQ et al below.
// The 'switch' is used to prevent the 'else' from being ambiguous when the
// macro is used in an 'if' clause such as:
// if (a == 1)
//   CHECK_EQ(2, a);
#define CHECK_OP(name, op, val1, val2)									\
	switch (0) case 0: default:											\
		if (annety::CheckOpResult true_if_passed =						\
			annety::Check##name##Impl((val1), (val2),					\
				#val1 " " #op " " #val2))								\
		;																\
		else															\
			annety::LogMessage(__LINE__, __FILE__, annety::LOG_FATAL,	\
				true_if_passed.message()).stream()

#endif  // !(NDEBUG)

// Build the error message string.  This is separate from the "Impl"
// function template because it is not performance critical and so can
// be out of line, while the "Impl" code should be inline.
template<class t1, class t2>
std::string MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
	std::ostringstream ss;
	ss << names << " (";
	ss << v1 << " vs. " << v2;
	ss << ")";
	return ss.str();	// string move
}

// Commonly used instantiations of MakeCheckOpString<>. Explicitly instantiated
// in Logging.cc.
extern template std::string MakeCheckOpString<int, int>(
	const int&, const int&, const char* names);
extern template std::string MakeCheckOpString<unsigned long, unsigned long>(
	const unsigned long&, const unsigned long&, const char* names);
extern template std::string MakeCheckOpString<unsigned long, unsigned int>(
	const unsigned long&, const unsigned int&, const char* names);
extern template std::string MakeCheckOpString<unsigned int, unsigned long>(
	const unsigned int&, const unsigned long&, const char* names);
extern template std::string MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);

// Helper functions for CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
//
// The checked condition is wrapped with ANALYZER_ASSUME_TRUE, which under
// static analysis builds, blocks analysis of the current path if the
// condition is false.
#define DEFINE_CHECK_OP_IMPL(name, op)											\
	template <class t1, class t2>												\
	inline std::string Check##name##Impl(const t1& v1, const t2& v2,			\
										const char* names) {					\
		if (ANALYZER_ASSUME_TRUE(v1 op v2))										\
			return std::string();												\
		else																	\
			return annety::MakeCheckOpString(v1, v2, names);					\
	}																			\
	inline std::string Check##name##Impl(int v1, int v2, const char* names) {	\
		if (ANALYZER_ASSUME_TRUE(v1 op v2))										\
			return std::string();												\
		else																	\
			return annety::MakeCheckOpString(v1, v2, names);					\
	}
DEFINE_CHECK_OP_IMPL(EQ, ==)
DEFINE_CHECK_OP_IMPL(NE, !=)
DEFINE_CHECK_OP_IMPL(LE, <=)
DEFINE_CHECK_OP_IMPL(LT, < )
DEFINE_CHECK_OP_IMPL(GE, >=)
DEFINE_CHECK_OP_IMPL(GT, > )
#undef DEFINE_CHECK_OP_IMPL

#define CHECK_EQ(val1, val2) CHECK_OP(EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(GT, > , val1, val2)

// DCHECK* / DLOG*------------------------------------------------------------

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

// Definitions for DLOG et al.
#if DCHECK_IS_ON()
#define DLOG_IS_ON(severity) LOG_IS_ON(severity)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DPLOG_IF(severity, condition) PLOG_IF(severity, condition)
#else

// If !DCHECK_IS_ON(), we want to avoid emitting any references to |condition|
// (which may reference a variable defined only if DCHECK_IS_ON()).
// Contrast this with DCHECK et al., which has different behavior.
#define DLOG_IS_ON(severity) false
#define DLOG_IF(severity, condition) EAT_STREAM_PARAMETERS
#define DPLOG_IF(severity, condition) EAT_STREAM_PARAMETERS
#endif  // DCHECK_IS_ON()


#define DLOG(severity)                                          \
	LAZY_STREAM(LOG_STREAM(severity), DLOG_IS_ON(severity))
#define DPLOG(severity)                                         \
	LAZY_STREAM(PLOG_STREAM(severity, GET_LAST_ERRNO()), 		\
		DLOG_IS_ON(severity))


// Definitions for DCHECK et al.
#if DCHECK_IS_ON()
const LogSeverity LOG_DCHECK = LOG_FATAL;
#else

// There may be users of LOG_DCHECK that are enabled independently
// of DCHECK_IS_ON(), so default to FATAL logging for those.
const LogSeverity LOG_DCHECK = LOG_FATAL;
#endif  // DCHECK_IS_ON()

// DCHECK et al. make sure to reference |condition| regardless of
// whether DCHECKs are enabled; this is so that we don't get unused
// variable warnings if the only use of a variable is in a DCHECK.
// This behavior is different from DLOG_IF et al.
//
// Note that the definition of the DCHECK macros depends on whether or not
// DCHECK_IS_ON() is true. When DCHECK_IS_ON() is false, the macros use
// EAT_STREAM_PARAMETERS to avoid expressions that would create temporaries.
#if DCHECK_IS_ON()
#define CHECK(condition)						\
	LAZY_STREAM(LOG_STREAM(FATAL, #condition), 	\
		!ANALYZER_ASSUME_TRUE(condition))
#define PCHECK(condition)											\
	LAZY_STREAM(PLOG_STREAM(FATAL, #condition, GET_LAST_ERRNO()),	\
		!ANALYZER_ASSUME_TRUE(condition))

#define DCHECK(condition)						\
	LAZY_STREAM(LOG_STREAM(DCHECK, #condition), \
		!ANALYZER_ASSUME_TRUE(condition))
#define DPCHECK(condition)											\
	LAZY_STREAM(PLOG_STREAM(DCHECK, #condition, GET_LAST_ERRNO()),	\
		!ANALYZER_ASSUME_TRUE(condition))
#else  // DCHECK_IS_ON()

#define DCHECK(condition) EAT_STREAM_PARAMETERS << !(condition)
#define DPCHECK(condition) EAT_STREAM_PARAMETERS << !(condition)
#endif  // DCHECK_IS_ON()


// Helper macro for binary operators.
// Don't use this macro directly in your code, use DCHECK_EQ et al below.
// The 'switch' is used to prevent the 'else' from being ambiguous when the
// macro is used in an 'if' clause such as:
// if (a == 1)
//   DCHECK_EQ(2, a);
#if DCHECK_IS_ON()
#define DCHECK_OP(name, op, val1, val2)									\
	switch (0) case 0: default:											\
		if (annety::CheckOpResult true_if_passed =						\
			annety::Check##name##Impl((val1), (val2),					\
				#val1 " " #op " " #val2))								\
		;																\
		else															\
			annety::LogMessage(__LINE__, __FILE__, annety::LOG_DCHECK,	\
				true_if_passed.message()).stream()
#else  // DCHECK_IS_ON()

// When DCHECKs aren't enabled, DCHECK_OP still needs to reference operator<<
// overloads for |val1| and |val2| to avoid potential compiler warnings about
// unused functions. For the same reason, it also compares |val1| and |val2|
// using |op|.
//
// Note that the contract of DCHECK_EQ, etc is that arguments are only evaluated
// once. Even though |val1| and |val2| appear twice in this version of the macro
// expansion, this is OK, since the expression is never actually evaluated.
#define DCHECK_OP(name, op, val1, val2)	\
	EAT_STREAM_PARAMETERS << ((val1)op(val2))

#endif  // DCHECK_IS_ON()

// Equality/Inequality checks - compare two values, and log a
// LOG_DCHECK message including the two values when the result is not
// as expected.  The values must have operator<<(ostream, ...)
// defined.
//
// You may append to the error message like so:
//   DCHECK_NE(1, 2) << "The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   DCHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL.  In new code, prefer nullptr instead.  To
// work around this for C++98, simply static_cast NULL to the type of the
// desired pointer.

#define DCHECK_EQ(val1, val2) DCHECK_OP(EQ, ==, val1, val2)
#define DCHECK_NE(val1, val2) DCHECK_OP(NE, !=, val1, val2)
#define DCHECK_LE(val1, val2) DCHECK_OP(LE, <=, val1, val2)
#define DCHECK_LT(val1, val2) DCHECK_OP(LT, < , val1, val2)
#define DCHECK_GE(val1, val2) DCHECK_OP(GE, >=, val1, val2)
#define DCHECK_GT(val1, val2) DCHECK_OP(GT, > , val1, val2)

// Definitions for NOTREACHED -----------------------------------------------
#if !DCHECK_IS_ON()
// Implement logging of NOTREACHED() as a dedicated function to get function
// call overhead down to a minimum.
void LogErrorNotReached(int line, const LogMessage::Filename& file);
#define NOTREACHED()                                       \
	true ? annety::LogErrorNotReached(__LINE__, __FILE__) \
		: EAT_STREAM_PARAMETERS
#else
#define NOTREACHED() DCHECK(false)
#endif	// DCHECK_IS_ON()


}	// namespace annety

#endif	// ANT_LOGGING_H
