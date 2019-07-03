// Refactoring: Anny Wang
// Date: Jul 02 2019

#include "LogFile.h"
#include "StringPrintf.h"
#include "Time.h"
#include "File.h"
#include "FilePath.h"

#include <assert.h>
#include <unistd.h>

namespace annety
{
namespace
{
pid_t pid()
{
	return ::getpid();
}

std::string hostname()
{
	// HOST_NAME_MAX 64
	// _POSIX_HOST_NAME_MAX 255
	char buf[256];
	if (::gethostname(buf, sizeof buf) == 0) {
		buf[sizeof(buf)-1] = '\0';
		return buf;
	} else {
		return "unknownhost";
	}
}

// basename.Ymd-HMS.hostname.pid.log
std::string get_log_filename(const std::string& basename, Time& format_tm)
{
	// basename
	std::string filename = basename;

	// %Y%m%d-%H%M%S
	Time::Exploded exploded;
	format_tm = Time::now();
	format_tm.to_local_explode(&exploded);
	string_appendf(&filename, ".%04d%02d%02d-%02d%02d%02d.",
								exploded.year,
								exploded.month,
								exploded.day_of_month,
								exploded.hour,
								exploded.minute,
								exploded.second);

	// hostname().pid().log
	string_appendf(&filename, "%s.%d.%s", hostname().c_str(), pid(), "log");

	return filename;
}

}	// namespace anonymous

LogFile::LogFile(const std::string& basename,
				off_t rotate_size_b,
				int flush_interval_s,
				int check_every_n,
				bool thread_safe)
	: basename_(basename),
	  rotate_size_b_(rotate_size_b),
	  check_every_n_(check_every_n),
	  flush_interval_s_(TimeDelta::from_seconds(flush_interval_s)),
	  lock_(thread_safe ? new MutexLock() : nullptr)
{
	assert(basename.find('/') == std::string::npos);
	rotate();
}

LogFile::~LogFile() = default;

void LogFile::append(const StringPiece& message)
{
	append(message.data(), message.size());
}

void LogFile::append(const char* message, int len)
{
	if (lock_) {
		AutoLock locked(*lock_);
		append_unlocked(message, len);
	} else {
		append_unlocked(message, len);
	}
}

void LogFile::flush()
{
	if (lock_) {
		AutoLock locked(*lock_);
		file_->flush();
	} else {
		file_->flush();
	}
}

void LogFile::append_unlocked(const char* message, int len)
{
	assert(file_->write_at_current_pos(message, len) == len);
	written_ += len;

	if (written_ > rotate_size_b_) {
		rotate();
	} else {
		++count_;
		if (count_ >= check_every_n_) {
			count_ = 0;

			Time curr = Time::now();
			if (curr.utc_midnight() != start_period_) {
				// one new day.
				rotate();
			} else if (curr - last_flush_ > flush_interval_s_) {
				last_flush_ = curr;
				file_->flush();
			}
		}
	}
}

bool LogFile::rotate()
{
	Time curr;
	std::string filename = get_log_filename(basename_, curr);

	if (curr.utc_midnight() > last_rotate_) {
		// one new day.
		if (file_) {
			file_->flush();
		}
		last_rotate_ = curr;
		last_flush_ = curr;
		start_period_ = curr.utc_midnight();
		
		file_.reset(new File(FilePath(filename), 
								File::FLAG_OPEN_ALWAYS | File::FLAG_APPEND));
		return true;
	}
	return false;
}

}	// namespace annety
