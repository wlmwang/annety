// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#include "Macros.h"
#include "Logging.h"
#include "TimeStamp.h"
#include "SafeStrerror.h"
#include "strings/StringPrintf.h"
#include "threading/ThreadForward.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>	// fwrite,abort
#include <stdlib.h>

namespace annety
{
namespace
{
const char* const kLogSeverityNames[] = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
static_assert(LOG_NUM_SEVERITIES == arraysize(kLogSeverityNames),
			"Incorrect number of kLogSeverityNames");

const char* log_severity_name(int severity)
{
	if (severity >= 0 && severity < LOG_NUM_SEVERITIES) {
		return kLogSeverityNames[severity];
	}
	return "UNKNOWN";
}

void defaultOutput(const char* msg, int len)
{
	size_t n = ::fwrite(msg, 1, len, stdout);
	ALLOW_UNUSED_LOCAL(n);
}

void defaultFlush()
{
	::fflush(stdout);
}

// using std::placeholders::_1;
// using std::placeholders::_2;
LogOutputHandlerFunction g_log_output_handler = &defaultOutput;
LogFFlushHandlerFunction g_log_fflush_handler = &defaultFlush;

// Any log at or above this level will be displayed. Anything below this level 
// will be silently ignored.
// 0=TRACE 1=INFO 2=WARNING 3=ERROR 4=FATAL
LogSeverity g_min_log_severity = 0;

// For cache colums logging
// FIXME: destruct not control
thread_local TimeStamp::Exploded tls_local_exploded{};
thread_local std::string tls_format_ymdhis{};
thread_local int64_t tls_last_second{};

}	// namespace anonymous

void set_min_log_severity(LogSeverity severity)
{
	g_min_log_severity = std::min(LOG_FATAL, (int)severity);
}
LogSeverity get_min_log_severity()
{
	return g_min_log_severity;
}
bool should_logging_message(LogSeverity severity)
{
	return severity >= g_min_log_severity;
}

LogOutputHandlerFunction set_log_output_handler(LogOutputHandlerFunction handler)
{
	LogOutputHandlerFunction rt_handler = g_log_output_handler;
	g_log_output_handler = handler;
	return rt_handler;
}
LogFFlushHandlerFunction set_log_fflush_handler(LogFFlushHandlerFunction handler)
{
	LogFFlushHandlerFunction rt_handler = g_log_fflush_handler;
	g_log_fflush_handler = handler;
	return rt_handler;
}

LogMessage::Impl::Impl(int line, const Filename& file, LogSeverity sev, int err)
	: line_(line), file_(file), severity_(sev), errno_(err)
{
	begin();
	
	// strerror
	if (errno_ != 0) {
		stream_ << fast_safe_strerror(errno_) << " (errno=" << errno_ << ") ";
	}
}

LogMessage::Impl::Impl(int line, const Filename& file, LogSeverity sev, 
						const std::string& msg, int err) 
	: line_(line), file_(file), severity_(sev), errno_(err)
{
	begin();
	stream_ << "Check failed: " << msg << ".";
	
	// strerror
	if (errno_ != 0) {
		stream_ << fast_safe_strerror(errno_) << " (errno=" << errno_ << ") ";
	}
}

void LogMessage::Impl::begin()
{
	// time exploded string
	TimeDelta td = time_ - TimeStamp();
	if (td.in_seconds() != tls_last_second) {
		time_.to_local_explode(&tls_local_exploded);
		sstring_printf(&tls_format_ymdhis, "%04d-%02d-%02d %02d:%02d:%02d",
						tls_local_exploded.year,
						tls_local_exploded.month,
						tls_local_exploded.day_of_month,
						tls_local_exploded.hour,
						tls_local_exploded.minute,
						tls_local_exploded.second);
		tls_last_second = td.in_seconds();
	}
	stream_ << string_printf("%s.%06d ", tls_format_ymdhis.c_str(), 
				static_cast<int>(td.internal_value() % TimeStamp::kMicrosecondsPerSecond));
	
	// tid string
	stream_ << threads::tid_string() << " ";

	// severity string
	stream_ << log_severity_name(severity_) << " ";
}

void LogMessage::Impl::endl()
{
	stream_ << " - " 
			<< StringPiece(file_.basename_, file_.size_) << ':' << line_ 
			<< '\n';
}

// {,D}LOG
LogMessage::LogMessage(int line, const Filename& file, LogSeverity sev)
	: LogMessage(line, file, sev, 0) {}

// {,D,P,DP}LOG
LogMessage::LogMessage(int line, const Filename& file, LogSeverity sev, int err) 
	: impl_(line, file, sev, err) {}

// {,D}CHECK
LogMessage::LogMessage(int line, const Filename& file, LogSeverity sev, const std::string& msg)
	: LogMessage(line, file, sev, msg, 0) {}

// {,D,P,DP}CHECK
LogMessage::LogMessage(int line, const Filename& file, LogSeverity sev, 
						const std::string& msg, int err) 
	: impl_(line, file, sev, msg, err) {}

LogMessage::~LogMessage()
{
	impl_.endl();

	// log message output
	if (g_log_output_handler) {
		const ByteBuffer& bf(stream().buffer());
		g_log_output_handler(bf.begin_read(), bf.readable_bytes());
	}
	// i/o fflush
	if (severity() == LOG_FATAL) {
		if (g_log_fflush_handler) {
			g_log_fflush_handler();
		}
		::abort();
	}
}

// Explicit instantiations for commonly used comparisons.
template std::string MakeCheckOpString<int, int>(
	const int&, const int&, const char* names);
template std::string MakeCheckOpString<unsigned long, unsigned long>(
	const unsigned long&, const unsigned long&, const char* names);
template std::string MakeCheckOpString<unsigned long, unsigned int>(
	const unsigned long&, const unsigned int&, const char* names);
template std::string MakeCheckOpString<unsigned int, unsigned long>(
	const unsigned int&, const unsigned long&, const char* names);
template std::string MakeCheckOpString<std::string, std::string>(
	const std::string&, const std::string&, const char* name);

// for NOTREACHED
void LogErrorNotReached(int line, const LogMessage::Filename& file)
{
	LogMessage(line , file, LOG_ERROR).stream() 
		<< "NOTREACHED() hit.";
}

}	// namespace annety

