// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 05 2019

#include "File.h"
#include "FilePath.h"
#include "Logging.h"
#include "EintrWrapper.h"

#include <unistd.h>		// pread,read,lseek,ftruncate
#include <errno.h>		// errno
#include <fcntl.h>		// fcntl
#include <sys/stat.h>	// fstat,S_ISDIR
#include <time.h>
#include <sys/time.h>	// futimes,timeval

namespace annety
{
// Make sure our Whence mappings match the system headers.
static_assert(File::FROM_BEGIN == SEEK_SET && File::FROM_CURRENT == SEEK_CUR &&
			  File::FROM_END == SEEK_END,
			  "whence mapping must match the system headers");

static_assert(sizeof(int64_t) == sizeof(off_t), "off_t must be 64 bits");

namespace
{
bool is_open_append(PlatformFile file)
{
	return (::fcntl(file, F_GETFL) & O_APPEND) != 0;
}

#if defined(OS_BSD) || defined(OS_MACOSX)
int call_fstat(int fd, stat_wrapper_t *st)
{
	return ::fstat(fd, st);
}
#else
int call_fstat(int fd, stat_wrapper_t *st)
{
	return ::fstat64(fd, st);
}
#endif  // defined(OS_BSD) || defined(OS_MACOSX)

int call_ftruncate(PlatformFile file, int64_t length)
{
	return HANDLE_EINTR(::ftruncate(file, length));
}

int call_futimes(PlatformFile file, const struct timeval times[2])
{
#ifdef __USE_XOPEN2K8
	// futimens should be available, but futimes might not be
	// http://pubs.opengroup.org/onlinepubs/9699919799/
	struct timespec ts_times[2];
	ts_times[0].tv_sec  = times[0].tv_sec;
	ts_times[0].tv_nsec = times[0].tv_usec * 1000;
	ts_times[1].tv_sec  = times[1].tv_sec;
	ts_times[1].tv_nsec = times[1].tv_usec * 1000;

	return ::futimens(file, ts_times);
#else
	return ::futimes(file, times);
#endif  // __USE_XOPEN2K8
}

File::Error call_fcntl_flock(PlatformFile file, bool do_lock)
{
	struct flock lock;
	lock.l_type = do_lock ? F_WRLCK : F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;  // Lock entire file.
	if (HANDLE_EINTR(::fcntl(file, F_SETLK, &lock)) == -1) {
		return File::get_last_file_error();
	}
	return File::FILE_OK;
}

}	// namespace anonymous

// File::Info ----------------------------------------------------

File::Info::Info()
	: size(0),
	  is_directory(false),
	  is_symbolic_link(false) {}

File::Info::~Info() = default;

void File::Info::from_stat(const stat_wrapper_t& stat_info)
{
	is_directory = S_ISDIR(stat_info.st_mode);
	is_symbolic_link = S_ISLNK(stat_info.st_mode);
	size = stat_info.st_size;

#if defined(OS_LINUX)
	time_t last_modified_sec = stat_info.st_mtim.tv_sec;
	int64_t last_modified_nsec = stat_info.st_mtim.tv_nsec;
	time_t last_accessed_sec = stat_info.st_atim.tv_sec;
	int64_t last_accessed_nsec = stat_info.st_atim.tv_nsec;
	time_t creation_time_sec = stat_info.st_ctim.tv_sec;
	int64_t creation_time_nsec = stat_info.st_ctim.tv_nsec;
#elif defined(OS_MACOSX) || defined(OS_BSD)
	time_t last_modified_sec = stat_info.st_mtimespec.tv_sec;
	int64_t last_modified_nsec = stat_info.st_mtimespec.tv_nsec;
	time_t last_accessed_sec = stat_info.st_atimespec.tv_sec;
	int64_t last_accessed_nsec = stat_info.st_atimespec.tv_nsec;
	time_t creation_time_sec = stat_info.st_ctimespec.tv_sec;
	int64_t creation_time_nsec = stat_info.st_ctimespec.tv_nsec;
#else
	time_t last_modified_sec = stat_info.st_mtime;
	int64_t last_modified_nsec = 0;
	time_t last_accessed_sec = stat_info.st_atime;
	int64_t last_accessed_nsec = 0;
	time_t creation_time_sec = stat_info.st_ctime;
	int64_t creation_time_nsec = 0;
#endif

	last_modified =
		Time::from_time_t(last_modified_sec) +
			TimeDelta::from_microseconds(last_modified_nsec /
				Time::kNanosecondsPerMicrosecond);

	last_accessed =
		Time::from_time_t(last_accessed_sec) +
			TimeDelta::from_microseconds(last_accessed_nsec /
				Time::kNanosecondsPerMicrosecond);

	creation_time =
		Time::from_time_t(creation_time_sec) +
			TimeDelta::from_microseconds(creation_time_nsec /
				Time::kNanosecondsPerMicrosecond);
}

// File ----------------------------------------------------

File::File()
	: error_details_(FILE_ERROR_FAILED),
	  created_(false),
	  async_(false) {}

File::File(const FilePath& path, uint32_t flags)
	: error_details_(FILE_OK), created_(false), async_(false)
{
	initialize(path, flags);
}

File::File(PlatformFile platform_file) : File(platform_file, false) {}

File::File(PlatformFile platform_file, bool async)
	: file_(platform_file),
	  error_details_(FILE_OK),
	  created_(false),
	  async_(async)
{
#if defined(OS_POSIX)
	DCHECK_GE(platform_file, -1);
#endif
}

File::File(Error error_details)
	: error_details_(error_details),
	  created_(false),
	  async_(false) {}

File::File(File&& other)
	: file_(other.take_platform_file()),
	  error_details_(other.error_details()),
	  created_(other.created()),
	  async_(other.async_) {}

File::~File()
{
	this->close();
}

File& File::operator=(File&& other)
{
	this->close();
	set_platform_file(other.take_platform_file());
	error_details_ = other.error_details();
	created_ = other.created();
	async_ = other.async_;
	return *this;
}

void File::initialize(const FilePath& path, uint32_t flags)
{
	if (path.references_parent()) {
		errno = EACCES;
		error_details_ = FILE_ERROR_ACCESS_DENIED;
		return;
	}
	do_initialize(path, flags);
}

bool File::is_valid() const
{
	return file_.is_valid();
}

PlatformFile File::get_platform_file() const
{
	return file_.get();
}

PlatformFile File::take_platform_file()
{
	return file_.release();
}

void File::close()
{
	if (!is_valid()) {
		return;
	}
	file_.reset();
}

int64_t File::seek(Whence whence, int64_t offset)
{
	DCHECK(is_valid());

	return ::lseek(file_.get(), static_cast<off_t>(offset),
				   static_cast<int>(whence));
}

int File::read(int64_t offset, char* data, int size)
{
	DCHECK(is_valid());
	if (size < 0) {
		return -1;
	}

	int bytes_read = 0;
	int rv;
	do {
		rv = HANDLE_EINTR(::pread(file_.get(), data + bytes_read,
								  size - bytes_read, offset + bytes_read));
		if (rv <= 0) {
			break;
		}

		bytes_read += rv;
	} while (bytes_read < size);

	return bytes_read ? bytes_read : rv;
}

int File::read_at_current_pos(char* data, int size)
{
	DCHECK(is_valid());
	if (size < 0) {
	return -1;
	}

	int bytes_read = 0;
	int rv;
	do {
		rv = HANDLE_EINTR(::read(file_.get(), data + bytes_read, 
								 size - bytes_read));
		if (rv <= 0) {
			break;
		}

		bytes_read += rv;
	} while (bytes_read < size);

	return bytes_read ? bytes_read : rv;
}

int File::read_no_best_effort(int64_t offset, char* data, int size)
{
	DCHECK(is_valid());
	if (size < 0) {
		return -1;
	}

	return HANDLE_EINTR(::pread(file_.get(), data, size, offset));
}

int File::read_at_current_pos_no_best_effort(char* data, int size)
{
	DCHECK(is_valid());
	if (size < 0) {
		return -1;
	}

	return HANDLE_EINTR(::read(file_.get(), data, size));
}

int File::write(int64_t offset, const char* data, int size)
{
	// note: O_APPEND flags called open() will ignore offset
	if (is_open_append(file_.get())) {
		return write_at_current_pos(data, size);
	}

	DCHECK(is_valid());
	if (size < 0) {
		return -1;
	}

	int bytes_written = 0;
	int rv;
	do {
		rv = HANDLE_EINTR(::pwrite(file_.get(), data + bytes_written,
								   size - bytes_written, offset + bytes_written));
		if (rv <= 0) {
			break;
		}

		bytes_written += rv;
	} while (bytes_written < size);

	return bytes_written ? bytes_written : rv;
}

int File::write_at_current_pos(const char* data, int size)
{
	DCHECK(is_valid());
	if (size < 0) {
		return -1;
	}

	int bytes_written = 0;
	int rv;
	do {
		rv = HANDLE_EINTR(::write(file_.get(), data + bytes_written,
								  size - bytes_written));
		if (rv <= 0) {
			break;
		}

		bytes_written += rv;
	} while (bytes_written < size);

	return bytes_written ? bytes_written : rv;
}

int File::write_at_current_pos_no_best_effort(const char* data, int size)
{
	DCHECK(is_valid());
	if (size < 0) {
		return -1;
	}

	return HANDLE_EINTR(::write(file_.get(), data, size));
}

int64_t File::get_length()
{
	DCHECK(is_valid());

	stat_wrapper_t file_info;
	if (call_fstat(file_.get(), &file_info)) {
		return -1;
	}

	return file_info.st_size;
}

bool File::set_length(int64_t length)
{
	DCHECK(is_valid());

	return !call_ftruncate(file_.get(), length);
}

bool File::set_times(Time last_access_time, Time last_modified_time)
{
	DCHECK(is_valid());

	timeval times[2];
	times[0] = last_access_time.to_timeval();
	times[1] = last_modified_time.to_timeval();

	return !call_futimes(file_.get(), times);
}

bool File::get_info(Info* info)
{
	DCHECK(is_valid());

	stat_wrapper_t file_info;
	if (call_fstat(file_.get(), &file_info)) {
		return false;
	}

	info->from_stat(file_info);
	return true;
}

File::Error File::lock()
{
	DCHECK(is_valid());

	return call_fcntl_flock(file_.get(), true);
}

File::Error File::unlock()
{
	DCHECK(is_valid());

	return call_fcntl_flock(file_.get(), false);
}

File File::duplicate() const
{
	if (!is_valid()) {
		return File();
	}

	PlatformFile other_fd = HANDLE_EINTR(dup(get_platform_file()));
	if (other_fd == -1) {
		return File(File::get_last_file_error());
	}

	return File(other_fd, async());
}

// Static.
File::Error File::os_error_to_file_error(int saved_errno)
{
	switch (saved_errno) {
		case EACCES:
		case EISDIR:
		case EROFS:
		case EPERM:
			return FILE_ERROR_ACCESS_DENIED;
		case EBUSY:
		case ETXTBSY:
			return FILE_ERROR_IN_USE;
		case EEXIST:
			return FILE_ERROR_EXISTS;
		case EIO:
			return FILE_ERROR_IO;
		case ENOENT:
			return FILE_ERROR_NOT_FOUND;
		case ENFILE:  // fallthrough
		case EMFILE:
			return FILE_ERROR_TOO_MANY_OPENED;
		case ENOMEM:
			return FILE_ERROR_NO_MEMORY;
		case ENOSPC:
			return FILE_ERROR_NO_SPACE;
		case ENOTDIR:
			return FILE_ERROR_NOT_A_DIRECTORY;
		default:
			// UmaHistogramSparse("PlatformFile.UnknownErrors.Posix", saved_errno);
			// This function should only be called for errors.
			DCHECK_NE(0, saved_errno);
			return FILE_ERROR_FAILED;
	}
}

// TODO(erikkay): does it make sense to support FLAG_EXCLUSIVE_* here?
void File::do_initialize(const FilePath& path, uint32_t flags)
{
	DCHECK(!is_valid());

	int open_flags = 0;
	if (flags & FLAG_CREATE) {
		open_flags = O_CREAT | O_EXCL;
	}

	created_ = false;

	if (flags & FLAG_CREATE_ALWAYS) {
		DCHECK(!open_flags);
		DCHECK(flags & FLAG_WRITE);
		open_flags = O_CREAT | O_TRUNC;
	}

	if (flags & FLAG_OPEN_TRUNCATED) {
		DCHECK(!open_flags);
		DCHECK(flags & FLAG_WRITE);
		open_flags = O_TRUNC;
	}

	if (!open_flags && !(flags & FLAG_OPEN) && !(flags & FLAG_OPEN_ALWAYS)) {
		NOTREACHED();
		errno = EOPNOTSUPP;
		error_details_ = FILE_ERROR_FAILED;
		return;
	}

	if (flags & FLAG_WRITE && flags & FLAG_READ) {
		open_flags |= O_RDWR;
	} else if (flags & FLAG_WRITE) {
		open_flags |= O_WRONLY;
	} else if (!(flags & FLAG_READ) &&
			!(flags & FLAG_APPEND) &&
			!(flags & FLAG_OPEN_ALWAYS))
	{
		NOTREACHED();
	}

	if (flags & FLAG_TERMINAL_DEVICE) {
		open_flags |= O_NOCTTY | O_NDELAY;
	}

	if (flags & FLAG_APPEND && flags & FLAG_READ) {
		open_flags |= O_APPEND | O_RDWR;
	} else if (flags & FLAG_APPEND) {
		open_flags |= O_APPEND | O_WRONLY;
	}

	static_assert(O_RDONLY == 0, "O_RDONLY must equal zero");

	int mode = S_IRUSR | S_IWUSR;

	int descriptor = HANDLE_EINTR(::open(path.value().c_str(), open_flags, mode));

	if (flags & FLAG_OPEN_ALWAYS) {
		if (descriptor < 0) {
			open_flags |= O_CREAT;
			if (flags & FLAG_EXCLUSIVE_READ || flags & FLAG_EXCLUSIVE_WRITE) {
				open_flags |= O_EXCL;   // together with O_CREAT implies O_NOFOLLOW
			}

			descriptor = HANDLE_EINTR(::open(path.value().c_str(), open_flags, mode));
			if (descriptor >= 0) {
				created_ = true;
			}
		}
	}

	if (descriptor < 0) {
		error_details_ = File::get_last_file_error();
		return;
	}

	if (flags & (FLAG_CREATE_ALWAYS | FLAG_CREATE)) {
		created_ = true;
	}

	if (flags & FLAG_DELETE_ON_CLOSE) {
		::unlink(path.value().c_str());
	}

	async_ = ((flags & FLAG_ASYNC) == FLAG_ASYNC);
	error_details_ = FILE_OK;
	file_.reset(descriptor);
}

bool File::flush()
{
	DCHECK(is_valid());

#if defined(OS_LINUX)
	return !HANDLE_EINTR(::fdatasync(file_.get()));
#else
	return !HANDLE_EINTR(::fsync(file_.get()));
#endif
}

void File::set_platform_file(PlatformFile file)
{
	DCHECK(!file_.is_valid());
	file_.reset(file);
}

// static
File::Error File::get_last_file_error()
{
	return File::os_error_to_file_error(errno);
}

std::string File::error_to_string(Error error)
{
	switch (error) {
		case FILE_OK:
			return "FILE_OK";
		case FILE_ERROR_FAILED:
			return "FILE_ERROR_FAILED";
		case FILE_ERROR_IN_USE:
			return "FILE_ERROR_IN_USE";
		case FILE_ERROR_EXISTS:
			return "FILE_ERROR_EXISTS";
		case FILE_ERROR_NOT_FOUND:
			return "FILE_ERROR_NOT_FOUND";
		case FILE_ERROR_ACCESS_DENIED:
			return "FILE_ERROR_ACCESS_DENIED";
		case FILE_ERROR_TOO_MANY_OPENED:
			return "FILE_ERROR_TOO_MANY_OPENED";
		case FILE_ERROR_NO_MEMORY:
			return "FILE_ERROR_NO_MEMORY";
		case FILE_ERROR_NO_SPACE:
			return "FILE_ERROR_NO_SPACE";
		case FILE_ERROR_NOT_A_DIRECTORY:
			return "FILE_ERROR_NOT_A_DIRECTORY";
		case FILE_ERROR_INVALID_OPERATION:
			return "FILE_ERROR_INVALID_OPERATION";
		case FILE_ERROR_SECURITY:
			return "FILE_ERROR_SECURITY";
		case FILE_ERROR_ABORT:
			return "FILE_ERROR_ABORT";
		case FILE_ERROR_NOT_A_FILE:
			return "FILE_ERROR_NOT_A_FILE";
		case FILE_ERROR_NOT_EMPTY:
			return "FILE_ERROR_NOT_EMPTY";
		case FILE_ERROR_INVALID_URL:
			return "FILE_ERROR_INVALID_URL";
		case FILE_ERROR_IO:
			return "FILE_ERROR_IO";
		case FILE_ERROR_MAX:
			break;
	}

	NOTREACHED();
	return "";
}

}	// namespace annety
