// Refactoring: Anny Wang
// Date: Jul 02 2019

#include "LogFile.h"
#include "Times.h"
#include "Logging.h"
#include "files/File.h"
#include "files/FilePath.h"
#include "strings/StringPrintf.h"

#include <assert.h>
#include <unistd.h>

namespace annety
{
namespace
{
pid_t getpid()
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

	// hostname().getpid().log
	string_appendf(&filename, "%s.%d.%s", hostname().c_str(), getpid(), "log");

	return FilePath(filename);
}

}	// namespace anonymous

LogFile::LogFile(const FilePath& path,
				off_t rotate_size_b,
				int check_every_n)
	: path_(path),
	  rotate_size_b_(rotate_size_b),
	  check_every_n_(check_every_n)
{
	assert(!path_.ends_with_separator());
	rotate(true);
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
	if (file_) {
		file_->flush();
	}
}

void LogFile::append_unlocked(const char* message, int len)
{
	if (written_.fetch_add(len) > rotate_size_b_) {
		rotate(true);
	} else {
		if (count_.fetch_add(1) >= check_every_n_) {
			count_.store(0);
			rotate();
		}
	}
	assert(file_->write_at_current_pos(message, len) == len);
}

void LogFile::rotate(bool force)
{
	auto do_rotate_with_locked = [this](const Time& curr) {
		lock_.assert_acquired();
		flush();

		File* file = new File(get_log_filename(path_), File::FLAG_OPEN_ALWAYS |
													   File::FLAG_APPEND);
		
		assert(file->error_details() == File::FILE_OK);
		written_.store(file->get_length());

		file_.reset(file);
		last_rotate_ = curr;
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
		} else {
			flush();
		}
	}
}

}	// namespace annety
