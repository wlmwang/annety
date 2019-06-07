// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: May 08 2019

#include "Logging.h"
#include "Macros.h"			// arraysize
#include "SafeStrerror.h"
#include "StringPrintf.h"
#include "Time.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>	// fwrite,abort
#include <stdlib.h>

namespace annety {
namespace {
const char* const kLogSeverityNames[4] = {"INFO", "WARNING", "ERROR", "FATAL"};
static_assert(LOG_NUM_SEVERITIES == arraysize(kLogSeverityNames),
			"Incorrect number of kLogSeverityNames");

const char* log_severity_name(int severity) {
	if (severity >= 0 && severity < LOG_NUM_SEVERITIES) {
		return kLogSeverityNames[severity];
	}
	return "UNKNOWN";
}

void defaultOutput(const char* msg, int len) {
	size_t n = ::fwrite(msg, 1, len, stdout);
	ALLOW_UNUSED_LOCAL(n);
}

void defaultFlush() {
	::fflush(stdout);
}

using std::placeholders::_1;
using std::placeholders::_2;
LogOutputHandlerFunction g_log_output_handler = std::bind(defaultOutput, _1, _2);
LogFFlushHandlerFunction g_log_fflush_handler = std::bind(defaultFlush);

// Any log at or above this level will be displayed. Anything below this level 
// will be silently ignored.
// 0=INFO 1=WARNING 2=ERROR 3=FATAL
LogSeverity g_min_log_level = 0;

// For cache colums logging
thread_local Time::Exploded t_local_exploded{};
thread_local std::string t_format_ymdhis{};
thread_local int64_t t_last_second{};
}	// namespace anonymous

void SetMinLogLevel(int level) {
	g_min_log_level = std::min(LOG_FATAL, level);
}
int GetMinLogLevel() {
	return g_min_log_level;
}
bool ShouldCreateLogMessage(int severity) {
	return severity >= g_min_log_level;
}

void SetLogOutputHandler(const LogOutputHandlerFunction& handler) {
	g_log_output_handler = handler;
}
void SetLogFFlushHandler(const LogFFlushHandlerFunction& handler) {
	g_log_fflush_handler = handler;
}

LogMessage::Impl::Impl(int line, const Filename& file, LogSeverity severity, int error)
	: line_(line), file_(file), severity_(severity), errno_(error) {
	begin();
}

LogMessage::Impl::Impl(int line, const Filename& file, LogSeverity severity, const std::string& message) 
	: line_(line), file_(file), severity_(severity) {
	begin();
	stream_ << "Check failed: " << message;
}

void LogMessage::Impl::begin() {
	// time exploded string
	TimeDelta td = time_ - Time();
	if (td.in_seconds() != t_last_second) {
		time_.to_local_explode(&t_local_exploded);
		sstring_printf(&t_format_ymdhis, "%04d-%02d-%02d %02d:%02d:%02d",
						t_local_exploded.year,
						t_local_exploded.month,
						t_local_exploded.day_of_month,
						t_local_exploded.hour,
						t_local_exploded.minute,
						t_local_exploded.second);
		t_last_second = td.in_seconds();
	}
	stream_ << string_printf("%s.%06d ", t_format_ymdhis.c_str(), 
				static_cast<int>(td.internal_value() % Time::kMicrosecondsPerSecond));
	// tid string

	// severity string
	stream_ << log_severity_name(severity_) << " ";

	// strerror
	if (errno_ != 0) {
		stream_ << fast_safe_strerror(errno_) << " (errno=" << errno_ << ") ";
	}
}

void LogMessage::Impl::endl() {
	stream_ << " - " << StringPiece(file_.basename_, file_.size_) << ':' << line_ << '\n';
}

// LogMessage::LogMessage(SourceFile file, int line)
// 	: impl_(INFO, 0, file, line) {}

// Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
//   : impl_(level, 0, file, line)
// {
//   impl_.stream_ << func << ' ';
// }

LogMessage::LogMessage(int line, const Filename& file, LogSeverity severity, 
	int error) : impl_(line, file, severity, error) {}

LogMessage::LogMessage(int line, const Filename& file, const std::string& result)
	: impl_(line, file, LOG_FATAL, result) {}

LogMessage::LogMessage(int line, const Filename& file, LogSeverity severity, 
	const std::string& result) : impl_(line, file, severity, result) {}

// LogMessage::LogMessage(const char* file, int line, const char* condition)
//     : severity_(LOG_FATAL), file_(file), line_(line) {
//   Init(file, line);
//   stream_ << "Check failed: " << condition << ". ";
// }

// LogMessage::LogMessage(const char* file, int line, std::string* result)
//     : severity_(LOG_FATAL), file_(file), line_(line) {
//   Init(file, line);
//   stream_ << "Check failed: " << *result;
//   delete result;
// }

// LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
//                        std::string* result)
//     : severity_(severity), file_(file), line_(line) {
//   Init(file, line);
//   stream_ << "Check failed: " << *result;
//   delete result;
// }

// Logger::Logger(SourceFile file, int line, bool toAbort)
//   : impl_(toAbort?FATAL:ERROR, errno, file, line)
// {
// }

LogMessage::~LogMessage() {
	impl_.endl();

	// log message output
	if (g_log_output_handler) {
		const LogBuffer& bf(stream().buffer());
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
void LogErrorNotReached(int line, const LogMessage::Filename& file) {
	LogMessage(line , file, LOG_ERROR).stream() 
		<< "NOTREACHED() hit.";
}

}	// namespace annety

