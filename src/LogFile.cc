// By: wlmwang
// Date: Jul 02 2019

#include "LogFile.h"
#include "TimeStamp.h"
#include "Logging.h"
#include "files/File.h"
#include "files/FilePath.h"
#include "strings/StringPrintf.h"

#include <unistd.h> // pid_t,getpid

namespace annety
{
namespace {
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
	TimeStamp::Exploded exploded;
	TimeStamp curr = TimeStamp::now();
	curr.to_local_explode(&exploded);
	sstring_appendf(&filename, 
		".%04d%02d%02d-%02d%02d%02d.",
		exploded.year,
		exploded.month,
		exploded.day_of_month,
		exploded.hour,
		exploded.minute,
		exploded.second
	);

	// hostname().getpid().log
	sstring_appendf(&filename, "%s.%d.%s", hostname().c_str(), ::getpid(), "log");

	return FilePath(filename);
}

}	// namespace anonymous

LogFile::LogFile(const FilePath& path,
				 off_t rotate_size_b,
				 int check_every_n)
	: path_(path)
	, rotate_size_b_(rotate_size_b)
	, check_every_n_(check_every_n)
{
	DCHECK(!path_.ends_with_separator());
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
	DCHECK(file_->write_at_current_pos(message, len) == len);
}

void LogFile::rotate(bool force)
{
	auto do_rotate_with_locked = [this](const TimeStamp& curr) {
		lock_.assert_acquired();
		flush();

		File* file = new File(get_log_filename(path_), File::FLAG_OPEN_ALWAYS |
													   File::FLAG_APPEND);
		
		DCHECK(file->error_details() == File::FILE_OK);
		written_.store(file->get_length());

		file_.reset(file);
		last_rotate_ = curr;
	};

	TimeStamp curr = TimeStamp::now();
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
