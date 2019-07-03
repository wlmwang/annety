// Refactoring: Anny Wang
// Date: Jul 02 2019

#include "LogFile.h"
#include "Time.h"
#include "File.h"
#include "FilePath.h"
#include "Logging.h"
#include "StringPrintf.h"

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

// path.Ymd-HMS.hostname.pid.log
FilePath get_log_filename(const FilePath& path)
{
	// path
	std::string filename = path.value();

	// %Y%m%d-%H%M%S
	Time::Exploded exploded;
	Time curr = Time::now();
	curr.to_local_explode(&exploded);
	string_appendf(&filename, ".%04d%02d%02d-%02d%02d%02d.",
								exploded.year,
								exploded.month,
								exploded.day_of_month,
								exploded.hour,
								exploded.minute,
								exploded.second);

	// hostname().pid().log
	string_appendf(&filename, "%s.%d.%s", hostname().c_str(), pid(), "log");

	return FilePath(filename);
}

}	// namespace anonymous

LogFile::LogFile(const FilePath& path,
				off_t rotate_size_b,
				int flush_interval_s,
				int check_every_n)
	: path_(path),
	  rotate_size_b_(rotate_size_b),
	  check_every_n_(check_every_n),
	  flush_interval_s_(TimeDelta::from_seconds(flush_interval_s))
{
	assert(!path_.ends_with_separator());
	rotate();
}

LogFile::~LogFile() = default;

void LogFile::append(const StringPiece& message)
{
	append(message.data(), message.size());
}

void LogFile::append(const char* message, int len)
{
	append_unlocked(message, len);
}

void LogFile::flush()
{
	file_->flush();
}

void LogFile::append_unlocked(const char* message, int len)
{
	assert(file_->write_at_current_pos(message, len) == len);

	if (written_.fetch_add(len) > rotate_size_b_) {
		rotate(true);
	} else {
		if (count_.fetch_add(1) >= check_every_n_) {
			count_.store(0);

			// FIXME: non atomic
			Time curr = Time::now();
			if (curr - last_flush_ > flush_interval_s_) {
				file_->flush();
				last_flush_ = curr;
			}
			rotate();
		}
	}
}

void LogFile::rotate(bool force)
{
	auto do_rotate_with_locked = [=] (const Time& curr) {
		if (file_) {
			file_->flush();
		}
		written_.store(0);

		last_rotate_ = curr;
		last_flush_ = curr;
		file_.reset(new File(get_log_filename(path_), 
							 File::FLAG_OPEN_ALWAYS | File::FLAG_APPEND));
	};

	Time curr = Time::now();
	if (force) {
		AutoLock locked(lock_);
		do_rotate_with_locked(curr);
	} else {
		// one new day.
		AutoLock locked(lock_);
		if (curr.utc_midnight() > last_rotate_) {
			do_rotate_with_locked(curr);
		}
	}
}

}	// namespace annety
