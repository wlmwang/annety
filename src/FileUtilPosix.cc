// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// @Modify: 2016/10/18

#include "FileUtil.h"
#include "FilePath.h"
#include "BuildConfig.h"
#include "Logging.h"
#include "StlUtil.h"
#include "Time.h"
// #include "Singleton.h"
#include "EintrWrapper.h"
#include "StringSplit.h"
#include "StringUtil.h"
#include "StringPrintf.h"
#include "FileEnumerator.h"
#include "ScopedFile.h"

#if defined(OS_MACOSX)
#include <copyfile.h>
#include <AvailabilityMacros.h>
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <grp.h>

namespace annety {
namespace {
const char kTempFileNamePattern[] = "annety.temp.XXXXXX";

#if defined(OS_BSD) || defined(OS_MACOSX)
int call_stat(const char* path, stat_wrapper_t* sb) {
	return ::stat(path, sb);
}
int call_lstat(const char* path, stat_wrapper_t* sb) {
	return ::lstat(path, sb);
}
#else
int call_stat(const char* path, stat_wrapper_t* sb) {
	return ::stat64(path, sb);
}
int call_lstat(const char* path, stat_wrapper_t* sb) {
	return ::lstat64(path, sb);
}
#endif

// Helper for VerifyPathControlledByUser.
bool verify_specific_path_controlled_by_user(const FilePath& path,
											 uid_t owner_uid,
											 const std::set<gid_t>& group_gids) {
	stat_wrapper_t stat_info;
	if (call_lstat(path.value().c_str(), &stat_info) != 0) {
		DPLOG(ERROR) << "Failed to get information on path "
					 << path.value();
		return false;
	}

	if (S_ISLNK(stat_info.st_mode)) {
		DLOG(ERROR) << "Path " << path.value() << " is a symbolic link.";
		return false;
	}

	if (stat_info.st_uid != owner_uid) {
		DLOG(ERROR) << "Path " << path.value() << " is owned by the wrong user.";
		return false;
	}

	// todo
	if ((stat_info.st_mode & S_IWGRP) &&
		!ContainsKey(group_gids, stat_info.st_gid))
	{
		DLOG(ERROR) << "Path " << path.value()
					<< " is writable by an unprivileged group.";
		return false;
	}

	if (stat_info.st_mode & S_IWOTH) {
		DLOG(ERROR) << "Path " << path.value() << " is writable by any user.";
		return false;
	}

	return true;
}

// todo TempFileName
std::string temp_file_name() {
	return std::string(kTempFileNamePattern);
}

bool advance_enumerator_with_stat(FileEnumerator* traversal,
								  FilePath* out_next_path,
								  struct stat* out_next_stat) {
	DCHECK(out_next_path);
	DCHECK(out_next_stat);
	*out_next_path = traversal->next();
	if (out_next_path->empty()) {
		return false;
	}

	*out_next_stat = traversal->get_info().stat();
	return true;
}

bool copy_file_contents(File* infile, File* outfile) {
	static constexpr size_t kBufferSize = 32768;
	std::vector<char> buffer(kBufferSize);

	for (;;) {
		ssize_t bytes_read = infile->read_at_current_pos(buffer.data(), buffer.size());
		if (bytes_read < 0) {
			return false;
		}
		if (bytes_read == 0) {
			return true;
		}

		// Allow for partial writes
		ssize_t bytes_written_per_read = 0;
		do {
			ssize_t bytes_written_partial = outfile->write_at_current_pos(
						&buffer[bytes_written_per_read], bytes_read - bytes_written_per_read);
			
			if (bytes_written_partial < 0) {
				return false;
			}

			bytes_written_per_read += bytes_written_partial;
		} while (bytes_written_per_read < bytes_read);
	}

	NOTREACHED();
	return false;
}

bool do_copy_directory(const FilePath& from_path,
					   const FilePath& to_path,
					   bool recursive,
					   bool open_exclusive)
{
	// Some old callers of CopyDirectory want it to support wildcards.
	// After some discussion, we decided to fix those callers.
	// Break loudly here if anyone tries to do this.
	DCHECK(to_path.value().find('*') == std::string::npos);
	DCHECK(from_path.value().find('*') == std::string::npos);

	if (from_path.value().size() >= PATH_MAX) {
		return false;
	}

	// This function does not properly handle destinations within the source
	FilePath real_to_path = to_path;
	if (path_exists(real_to_path)) {
		real_to_path = make_absolute_filepath(real_to_path);
	} else {
		real_to_path = make_absolute_filepath(real_to_path.dir_name());
	}
	
	if (real_to_path.empty()) {
		return false;
	}

	FilePath real_from_path = make_absolute_filepath(from_path);
	if (real_from_path.empty()) {
		return false;
	}
	if (real_to_path == real_from_path || real_from_path.is_parent(real_to_path)) {
		return false;
	}

	int traverse_type = FileEnumerator::FILES | FileEnumerator::SHOW_SYM_LINKS;
	if (recursive) {
		traverse_type |= FileEnumerator::DIRECTORIES;
	}
	FileEnumerator traversal(from_path, recursive, traverse_type);

	// We have to mimic windows behavior here. |to_path| may not exist yet,
	// start the loop with |to_path|.
	struct stat from_stat;
	FilePath current = from_path;
	if (::stat(from_path.value().c_str(), &from_stat) < 0) {
		DPLOG(ERROR) << "CopyDirectory() couldn't stat source directory: "
					 << from_path.value();
		return false;
	}
	FilePath from_path_base = from_path;
	if (recursive && directory_exists(to_path)) {
		// If the destination already exists and is a directory, then the
		// top level of source needs to be copied.
		from_path_base = from_path.dir_name();
	}

	// The Windows version of this function assumes that non-recursive calls
	// will always have a directory for from_path.
	// TODO(maruel): This is not necessary anymore.
	DCHECK(recursive || S_ISDIR(from_stat.st_mode));

	do {
		// current is the source path, including from_path, so append
		// the suffix after from_path to to_path to create the target_path.
		FilePath target_path(to_path);
		if (from_path_base != current &&
			!from_path_base.append_relative_path(current, &target_path))
		{
			return false;
		}

		if (S_ISDIR(from_stat.st_mode)) {
			mode_t mode = (from_stat.st_mode & 01777) | S_IRUSR | S_IXUSR | S_IWUSR;
			if (::mkdir(target_path.value().c_str(), mode) == 0) {
				continue;
			}
			if (errno == EEXIST && !open_exclusive) {
				continue;
			}

			DPLOG(ERROR) << "CopyDirectory() couldn't create directory: "
						 << target_path.value();
			return false;
		}

		if (!S_ISREG(from_stat.st_mode)) {
			DLOG(WARNING) << "CopyDirectory() skipping non-regular file: "
						  << current.value();
			continue;
		}

		// Add O_NONBLOCK so we can't block opening a pipe.
		File infile(::open(current.value().c_str(), O_RDONLY | O_NONBLOCK));
		if (!infile.is_valid()) {
			DPLOG(ERROR) << "CopyDirectory() couldn't open file: " << current.value();
			return false;
		}

		struct stat stat_at_use;
		if (::fstat(infile.get_platform_file(), &stat_at_use) < 0) {
			DPLOG(ERROR) << "CopyDirectory() couldn't stat file: " << current.value();
			return false;
		}

		if (!S_ISREG(stat_at_use.st_mode)) {
			DLOG(WARNING) << "CopyDirectory() skipping non-regular file: "
						  << current.value();
			continue;
		}

		int open_flags = O_WRONLY | O_CREAT;
		// If |open_exclusive| is set then we should always create the destination
		// file, so O_NONBLOCK is not necessary to ensure we don't block on the
		// open call for the target file below, and since the destination will
		// always be a regular file it wouldn't affect the behavior of the
		// subsequent write calls anyway.
		if (open_exclusive) {
			open_flags |= O_EXCL;
		} else {
			open_flags |= O_TRUNC | O_NONBLOCK;
		}
		// Each platform has different default file opening modes for CopyFile which
		// we want to replicate here. On OS X, we use copyfile(3) which takes the
		// source file's permissions into account. On the other platforms, we just
		// use the base::File constructor. On Chrome OS, base::File uses a different
		// set of permissions than it does on other POSIX platforms.
		#if defined(OS_MACOSX)
		int mode = 0600 | (stat_at_use.st_mode & 0177);
		#else
		int mode = 0600;
		#endif
		File outfile(::open(target_path.value().c_str(), open_flags, mode));
		if (!outfile.is_valid()) {
			DPLOG(ERROR) << "CopyDirectory() couldn't create file: "
						 << target_path.value();
			return false;
		}

		if (!copy_file_contents(&infile, &outfile)) {
			DLOG(ERROR) << "CopyDirectory() couldn't copy file: " << current.value();
			return false;
		}
	} while (advance_enumerator_with_stat(&traversal, &current, &from_stat));

	return true;
}

#if !defined(OS_MACOSX)
// Appends |mode_char| to |mode| before the optional character set encoding; see
// https://www.gnu.org/software/libc/manual/html_node/Opening-Streams.html for
// details.
std::string append_mode_character(StringPiece mode, char mode_char) {
	std::string result(mode.as_string());
	size_t comma_pos = result.find(',');
	result.insert(comma_pos == std::string::npos ? result.length() : comma_pos, 1,
				  mode_char);
	return result;
}
#endif

}	// namespace anonymous


FilePath make_absolute_filepath(const FilePath& input) {
	char full_path[PATH_MAX];
	if (::realpath(input.value().c_str(), full_path) == nullptr) {
		return FilePath();
	}
	return FilePath(full_path);
}

// TODO(erikkay): The Windows version of this accepts paths like "foo/bar/*"
// which works both with and without the recursive flag.  I'm not sure we need
// that functionality. If not, remove from file_util_win.cc, otherwise add it
// here.
bool delete_file(const FilePath& path, bool recursive) {
	const char* path_str = path.value().c_str();
	stat_wrapper_t file_info;
	if (call_lstat(path_str, &file_info) != 0) {
		// The Windows version defines this condition as success.
		return (errno == ENOENT || errno == ENOTDIR);
	}
	if (!S_ISDIR(file_info.st_mode)) {
		return (::unlink(path_str) == 0);
	}
	if (!recursive) {
		return (::rmdir(path_str) == 0);
	}

	bool success = true;
	std::stack<std::string> directories;
	directories.push(path.value());
	FileEnumerator traversal(path, true,
		FileEnumerator::FILES | FileEnumerator::DIRECTORIES | 
		FileEnumerator::SHOW_SYM_LINKS);
	for (FilePath current = traversal.next(); !current.empty();
		current = traversal.next())
	{
		if (traversal.get_info().is_directory()) {
			directories.push(current.value());
		} else {
			success &= (::unlink(current.value().c_str()) == 0);
		}
	}

	while (!directories.empty()) {
		FilePath dir = FilePath(directories.top());
		directories.pop();
		success &= (::rmdir(dir.value().c_str()) == 0);
	}
	return success;
}

bool replace_file(const FilePath& from_path,
				  const FilePath& to_path,
				  File::Error* error)
{
	if (::rename(from_path.value().c_str(), to_path.value().c_str()) == 0) {
		return true;
	}
	if (error) {
		*error = File::get_last_file_error();
	}
	return false;
}

bool copy_directory(const FilePath& from_path,
					const FilePath& to_path,
					bool recursive)
{
	return do_copy_directory(from_path, to_path, recursive, false);
}

bool copy_directory_excl(const FilePath& from_path,
						 const FilePath& to_path,
						 bool recursive) {
	return do_copy_directory(from_path, to_path, recursive, true);
}

bool create_local_non_blocking_pipe(int fds[2]) {
#if defined(OS_LINUX)
	return 
		::pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0;
#else
	int raw_fds[2];
	if (::pipe(raw_fds) != 0) {
		return false;
	}

	ScopedFD fd_out(raw_fds[0]);
	ScopedFD fd_in(raw_fds[1]);
	if (!set_close_on_exec(fd_out.get())) {
		return false;
	}
	if (!set_close_on_exec(fd_in.get())) {
		return false;
	}
	if (!set_non_blocking(fd_out.get())) {
		return false;
	}
	if (!set_non_blocking(fd_in.get())) {
		return false;
	}
	fds[0] = fd_out.release();
	fds[1] = fd_in.release();
	return true;
#endif
}

bool set_non_blocking(int fd) {
	const int flags = ::fcntl(fd, F_GETFL);
	if (flags == -1) {
		return false;
	}
	if (flags & O_NONBLOCK) {
		return true;
	}
	if (HANDLE_EINTR(::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) == -1) {
		return false;
	}
	return true;
}

bool set_close_on_exec(int fd) {
	const int flags = ::fcntl(fd, F_GETFD);
	if (flags == -1) {
		return false;
	}
	if (flags & FD_CLOEXEC) {
		return true;
	}
	if (HANDLE_EINTR(::fcntl(fd, F_SETFD, flags | FD_CLOEXEC)) == -1) {
		return false;
	}
	return true;
}

bool path_exists(const FilePath& path) {
	return ::access(path.value().c_str(), F_OK) == 0;
}

bool path_is_writable(const FilePath& path) {
	return ::access(path.value().c_str(), W_OK) == 0;
}

bool directory_exists(const FilePath& path) {
	stat_wrapper_t file_info;
	if (call_stat(path.value().c_str(), &file_info) != 0) {
		return false;
	}
	return S_ISDIR(file_info.st_mode);
}

bool read_from_fd(int fd, char* buffer, size_t bytes) {
	size_t total_read = 0;
	while (total_read < bytes) {
		ssize_t bytes_read =
			HANDLE_EINTR(::read(fd, buffer + total_read, bytes - total_read));
		if (bytes_read <= 0) {
			break;
		}
		total_read += bytes_read;
	}
	return total_read == bytes;
}

int create_and_open_fd_for_temporary_file_in_dir(const FilePath& directory,
												 FilePath* path)
{
	// For call to mkstemp().
	*path = directory.append(temp_file_name());
	const std::string& tmpdir_string = path->value();
	// this should be OK since mkstemp just replaces characters in place
	char* buffer = const_cast<char*>(tmpdir_string.c_str());

	return HANDLE_EINTR(::mkstemp(buffer));
}

bool create_symbolic_link(const FilePath& target_path,
						  const FilePath& symlink_path)
{
	DCHECK(!symlink_path.empty());
	DCHECK(!target_path.empty());
	return ::symlink(target_path.value().c_str(),
					 symlink_path.value().c_str()) != -1;
}

bool read_symbolic_link(const FilePath& symlink_path, FilePath* target_path) {
	DCHECK(!symlink_path.empty());
	DCHECK(target_path);

	char buf[PATH_MAX];
	ssize_t count = ::readlink(symlink_path.value().c_str(), buf, annety::size(buf));

	if (count <= 0) {
		target_path->clear();
		return false;
	}

	*target_path = FilePath(FilePath::StringType(buf, count));
	return true;
}

bool get_posix_file_permissions(const FilePath& path, int* mode) {
  DCHECK(mode);

	stat_wrapper_t file_info;
	// Uses stat(), because on symbolic link, lstat() does not return valid
	// permission bits in st_mode
	if (call_stat(path.value().c_str(), &file_info) != 0) {
		return false;
	}

	*mode = file_info.st_mode & FILE_PERMISSION_MASK;
	return true;
}

bool SetPosixFilePermissions(const FilePath& path,
							 int mode)
{
	DCHECK_EQ(mode & ~FILE_PERMISSION_MASK, 0);

	// Calls stat() so that we can preserve the higher bits like S_ISGID.
	stat_wrapper_t stat_buf;
	if (call_stat(path.value().c_str(), &stat_buf) != 0) {
		return false;
	}

	// Clears the existing permission bits, and adds the new ones.
	mode_t updated_mode_bits = stat_buf.st_mode & ~FILE_PERMISSION_MASK;
	updated_mode_bits |= mode & FILE_PERMISSION_MASK;

	if (HANDLE_EINTR(::chmod(path.value().c_str(), updated_mode_bits)) != 0) {
		return false;
	}

	return true;
}


#if defined(OS_MACOSX)
bool get_temp_dir(FilePath* path) {
	// In order to facilitate hermetic runs on macOS, first check
	// $MAC_CHROMIUM_TMPDIR. We check this instead of $TMPDIR because external
	// programs currently set $TMPDIR with no effect, but when we respect it
	// directly it can cause crashes (like crbug.com/698759).
	const char* env_tmpdir = ::getenv("TMPDIR");
	if (env_tmpdir) {
		DCHECK_LT(::strlen(env_tmpdir), 50u)
				<< "too-long TMPDIR causes socket name length issues.";
		*path = FilePath(env_tmpdir);
		return true;
	}

	*path = FilePath("/tmp");
	return true;
}
#else

// This is implemented in file_util_mac.mm for Mac.
bool get_temp_dir(FilePath* path) {
	const char* tmp = ::getenv("TMPDIR");
	if (tmp) {
		*path = FilePath(tmp);
		return true;
	}

	*path = FilePath("/tmp");
	return true;
}
#endif  // defined(OS_MACOSX)

#if defined(OS_MACOSX)
FilePath get_home_dir() {
	const char* home_dir = ::getenv("HOME");
	if (home_dir && home_dir[0]) {
		return FilePath(home_dir);
	}

	// Fall back on temp dir if no home directory is defined.
	FilePath rv;
	if (get_temp_dir(&rv)) {
		return rv;
	}

	// Last resort.
	return FilePath("/tmp");
}
#else

FilePath GetHomeDir() {
	const char* home_dir = ::getenv("HOME");
	if (home_dir && home_dir[0]) {
		return FilePath(home_dir);
	}

	FilePath rv;
	if (get_temp_dir(&rv)) {
		return rv;
	}

	// Last resort.
	return FilePath("/tmp");
}
#endif  // !defined(OS_MACOSX)


bool create_temporary_file(FilePath* path) {
	// For call to close().
	FilePath directory;
	if (!get_temp_dir(&directory)) {
		return false;
	}

	int fd = create_and_open_fd_for_temporary_file_in_dir(directory, path);
	if (fd < 0) {
		return false;
	}
	
	::close(fd);
	return true;
}

FILE* create_and_open_temporary_file_in_dir(const FilePath& dir, FilePath* path) {
	int fd = create_and_open_fd_for_temporary_file_in_dir(dir, path);
	if (fd < 0) {
		return nullptr;
	}

	FILE* file = ::fdopen(fd, "a+");
	if (!file) {
		::close(fd);
	}
	return file;
}

bool create_temporary_file_in_dir(const FilePath& dir, FilePath* temp_file) {
	// For call to close().
	int fd = create_and_open_fd_for_temporary_file_in_dir(dir, temp_file);
	return ((fd >= 0) && !IGNORE_EINTR(::close(fd)));
}

static bool create_temporary_dir_in_dir_impl(const FilePath& base_dir,
											 const FilePath::StringType& name_tmpl,
											 FilePath* new_dir)
{
	// For call to mkdtemp().
	DCHECK(name_tmpl.find("XXXXXX") != FilePath::StringType::npos)
		<< "Directory name template must contain \"XXXXXX\".";

	FilePath sub_dir = base_dir.append(name_tmpl);
	std::string sub_dir_string = sub_dir.value();

	// this should be OK since mkdtemp just replaces characters in place
	char* buffer = const_cast<char*>(sub_dir_string.c_str());
	char* dtemp = ::mkdtemp(buffer);
	if (!dtemp) {
		DPLOG(ERROR) << "mkdtemp";
		return false;
	}
	*new_dir = FilePath(dtemp);
	return true;
}

bool create_temporary_dir_in_dir(const FilePath& base_dir,
								 const FilePath::StringType& prefix,
								 FilePath* new_dir)
{
	FilePath::StringType mkdtemp_template = prefix;
	mkdtemp_template.append("XXXXXX");
	return create_temporary_dir_in_dir_impl(base_dir, mkdtemp_template, new_dir);
}

bool create_new_temp_directory(const FilePath::StringType& prefix,
							   FilePath* new_temp_path)
{
	FilePath tmpdir;
	if (!get_temp_dir(&tmpdir)) {
		return false;
	}

	return create_temporary_dir_in_dir_impl(tmpdir, temp_file_name(), new_temp_path);
}

bool create_directory_and_get_error(const FilePath& full_path,
									File::Error* error)
{
	// For call to mkdir().
	std::vector<FilePath> subpaths;

	// Collect a list of all parent directories.
	FilePath last_path = full_path;
	subpaths.push_back(full_path);
	for (FilePath path = full_path.dir_name();
		path.value() != last_path.value(); path = path.dir_name())
	{
		subpaths.push_back(path);
		last_path = path;
	}

	// Iterate through the parents and create the missing ones.
	for (std::vector<FilePath>::reverse_iterator i = subpaths.rbegin();
		i != subpaths.rend(); ++i)
	{
		if (directory_exists(*i)) {
			continue;
		}
		if (mkdir(i->value().c_str(), 0700) == 0) {
			continue;
		}
		// Mkdir failed, but it might have failed with EEXIST, or some other error
		// due to the the directory appearing out of thin air. This can occur if
		// two processes are trying to create the same file system tree at the same
		// time. Check to see if it exists and make sure it is a directory.
		int saved_errno = errno;
		if (!directory_exists(*i)) {
			if (error) {
				*error = File::os_error_to_file_error(saved_errno);
			}
			return false;
		}
	}
	return true;
}

bool normalize_filepath(const FilePath& path, FilePath* normalized_path) {
	FilePath real_path_result = make_absolute_filepath(path);
	if (real_path_result.empty()) {
		return false;
	}

	// To be consistant with windows, fail if |real_path_result| is a
	// directory.
	if (directory_exists(real_path_result)) {
		return false;
	}

	*normalized_path = real_path_result;
	return true;
}

// TODO(rkc): Refactor GetFileInfo and FileEnumerator to handle symlinks
// correctly. http://code.google.com/p/chromium-os/issues/detail?id=15948
bool is_link(const FilePath& file_path) {
	stat_wrapper_t st;
	// If we can't lstat the file, it's safe to assume that the file won't at
	// least be a 'followable' link.
	if (call_lstat(file_path.value().c_str(), &st) != 0) {
		return false;
	}
	return S_ISLNK(st.st_mode);
}

bool get_file_info(const FilePath& file_path, File::Info* results) {
	stat_wrapper_t file_info;
	if (call_stat(file_path.value().c_str(), &file_info) != 0) {
		return false;
	}

	results->from_stat(file_info);
	return true;
}

FILE* open_file(const FilePath& filename, const char* mode) {
	// 'e' is unconditionally added below, so be sure there is not one already
	// present before a comma in |mode|.
	DCHECK(::strchr(mode, 'e') == nullptr ||
		  (::strchr(mode, ',') != nullptr && ::strchr(mode, 'e') > ::strchr(mode, ',')));
  
	FILE* result = nullptr;
#if defined(OS_MACOSX)
	// macOS does not provide a mode character to set O_CLOEXEC; see
	// https://developer.apple.com/legacy/library/documentation/Darwin/Reference/ManPages/man3/fopen.3.html.
	const char* the_mode = mode;
#else
	std::string mode_with_e(append_mode_character(mode, 'e'));
	const char* the_mode = mode_with_e.c_str();
#endif

	do {
		result = ::fopen(filename.value().c_str(), the_mode);
	} while (!result && errno == EINTR);

#if defined(OS_MACOSX)
	// Mark the descriptor as close-on-exec.
	if (result) {
		set_close_on_exec(::fileno(result));
	}
#endif

	return result;
}

FILE* file_to_file(File file, const char* mode) {
	FILE* stream = ::fdopen(file.get_platform_file(), mode);
	if (stream) {
		file.take_platform_file();
	}
	return stream;
}

int read_file(const FilePath& filename, char* data, int max_size) {
	int fd = HANDLE_EINTR(::open(filename.value().c_str(), O_RDONLY));
	if (fd < 0) {
		return -1;
	}

	ssize_t bytes_read = HANDLE_EINTR(::read(fd, data, max_size));
	if (IGNORE_EINTR(::close(fd)) < 0) {
		return -1;
	}
	return bytes_read;
}

int write_file(const FilePath& filename, const char* data, int size) {
	int fd = HANDLE_EINTR(::creat(filename.value().c_str(), 0666));
	if (fd < 0) {
		return -1;
	}

	int bytes_written = write_file_descriptor(fd, data, size) ? size : -1;
	if (IGNORE_EINTR(::close(fd)) < 0) {
		return -1;
	}
	return bytes_written;
}

bool write_file_descriptor(const int fd, const char* data, int size) {
	// Allow for partial writes.
	ssize_t bytes_written_total = 0;
	for (ssize_t bytes_written_partial = 0; bytes_written_total < size;
		bytes_written_total += bytes_written_partial)
	{
		bytes_written_partial =
			HANDLE_EINTR(::write(fd, data + bytes_written_total,
					size - bytes_written_total));
		if (bytes_written_partial < 0) {
			return false;
		}
	}

	return true;
}

bool append_to_file(const FilePath& filename, const char* data, int size) {
	bool ret = true;
	int fd = HANDLE_EINTR(::open(filename.value().c_str(), O_WRONLY | O_APPEND));
	if (fd < 0) {
		DLOG(ERROR) << "Unable to create file " << filename.value();
		return false;
	}

	// This call will either write all of the data or return false.
	if (!write_file_descriptor(fd, data, size)) {
		DLOG(ERROR) << "Error while writing to file " << filename.value();
		ret = false;
	}

	if (IGNORE_EINTR(::close(fd)) < 0) {
		DLOG(ERROR) << "Error while closing file " << filename.value();
		return false;
	}

	return ret;
}

bool get_current_directory(FilePath* dir) {
	// getcwd can return ENOENT, which implies it checks against the disk.
	char system_buffer[PATH_MAX] = "";
	if (!::getcwd(system_buffer, sizeof(system_buffer))) {
		NOTREACHED();
		return false;
	}
	*dir = FilePath(system_buffer);
	return true;
}

bool set_current_directory(const FilePath& path) {
	return ::chdir(path.value().c_str()) == 0;
}

bool verify_path_controlled_by_user(const FilePath& base,
									const FilePath& path,
									uid_t owner_uid,
									const std::set<gid_t>& group_gids)
{
	if (base != path && !base.is_parent(path)) {
		DLOG(ERROR) << "|base| must be a subdirectory of |path|.  base = \""
			<< base.value() << "\", path = \"" << path.value() << "\"";
		return false;
	}

	std::vector<FilePath::StringType> base_components;
	std::vector<FilePath::StringType> path_components;

	base.get_components(&base_components);
	path.get_components(&path_components);

	std::vector<FilePath::StringType>::const_iterator ib, ip;
	for (ib = base_components.begin(), ip = path_components.begin();
		ib != base_components.end(); ++ib, ++ip)
	{
		// |base| must be a subpath of |path|, so all components should match.
		// If these CHECKs fail, look at the test that base is a parent of
		// path at the top of this function.
		DCHECK(ip != path_components.end());
		DCHECK(*ip == *ib);
	}

	FilePath current_path = base;
	if (!verify_specific_path_controlled_by_user(current_path, owner_uid, group_gids)) {
		return false;
	}

	for (; ip != path_components.end(); ++ip) {
		current_path = current_path.append(*ip);
		if (!verify_specific_path_controlled_by_user(current_path, 
													 owner_uid,
													 group_gids))
		{
			return false;
		}
	}
	return true;
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
bool verify_path_controlled_by_admin(const FilePath& path)
{
	const unsigned kRootUid = 0;
	const FilePath kFileSystemRoot("/");

	// The name of the administrator group on mac os.
	const char* const kAdminGroupNames[] = {
		"admin",
		"wheel"
	};

	std::set<gid_t> allowed_group_ids;
	for (int i = 0, ie = annety::size(kAdminGroupNames); i < ie; ++i) {
		struct group *group_record = ::getgrnam(kAdminGroupNames[i]);
		if (!group_record) {
			DPLOG(ERROR) << "Could not get the group ID of group \""
						 << kAdminGroupNames[i] << "\".";
			continue;
		}

		allowed_group_ids.insert(group_record->gr_gid);
	}

	return verify_path_controlled_by_user(
		kFileSystemRoot, path, kRootUid, allowed_group_ids);
}
#endif  // defined(OS_MACOSX) && !defined(OS_IOS)

int get_maximum_path_component_length(const FilePath& path) {
	return ::pathconf(path.value().c_str(), _PC_NAME_MAX);
}

// This is implemented in file_util_android.cc for that platform.
bool GetShmemTempDir(bool executable, FilePath* path) {
#if defined(OS_LINUX) || defined(OS_AIX)
	bool disable_dev_shm = false;
	bool use_dev_shm = true;
	if (executable) {
		static const bool s_dev_shm_executable =
				is_path_executable(FilePath("/dev/shm"));
		use_dev_shm = s_dev_shm_executable;
	}
	if (use_dev_shm && !disable_dev_shm) {
		*path = FilePath("/dev/shm");
		return true;
	}
#endif  // defined(OS_LINUX) || defined(OS_AIX)

return get_temp_dir(path);
}


#if defined(OS_MACOSX)
bool copy_file(const FilePath& from_path, const FilePath& to_path) {
	if (from_path.references_parent() || to_path.references_parent()) {
		return false;
	}
	return (::copyfile(from_path.value().c_str(),
			to_path.value().c_str(), NULL, COPYFILE_DATA) == 0);
}
#else

// Mac has its own implementation, this is for all other Posix systems.
bool copy_file(const FilePath& from_path, const FilePath& to_path) {
	File infile;
	infile = File(from_path, File::FLAG_OPEN | File::FLAG_READ);
	if (!infile.is_valid()) {
		return false;
	}

	File outfile(to_path, File::FLAG_WRITE | File::FLAG_CREATE_ALWAYS);
	if (!outfile.is_valid()) {
		return false;
	}

	return copy_file_contents(&infile, &outfile);
}
#endif  // !defined(OS_MACOSX)

// -----------------------------------------------------------------------------

namespace internal {
bool move_unsafe(const FilePath& from_path, const FilePath& to_path) {
	// Windows compatibility: if |to_path| exists, |from_path| and |to_path|
	// must be the same type, either both files, or both directories.
	stat_wrapper_t to_file_info;
	if (call_stat(to_path.value().c_str(), &to_file_info) == 0) {
		stat_wrapper_t from_file_info;
		if (call_stat(from_path.value().c_str(), &from_file_info) != 0) {
			return false;
		}
		if (S_ISDIR(to_file_info.st_mode) != S_ISDIR(from_file_info.st_mode)) {
			return false;
		}
	}

	if (::rename(from_path.value().c_str(), to_path.value().c_str()) == 0) {
		return true;
	}

	if (!copy_directory(from_path, to_path, true)) {
		return false;
	}

	delete_file(from_path, true);
	return true;
}

}	// namespace internal

#if defined(OS_LINUX) || defined(OS_AIX)
WUTIL_EXPORT bool is_path_executable(const FilePath& path) {
	bool result = false;
	FilePath tmp_file_path;

	ScopedFD fd(create_and_open_fd_for_temporary_file_in_dir(path, &tmp_file_path));
	if (fd.is_valid()) {
		delete_file(tmp_file_path, false);

		long sysconf_result = ::sysconf(_SC_PAGESIZE);
		CHECK_GE(sysconf_result, 0);
		
		size_t pagesize = static_cast<size_t>(sysconf_result);
		CHECK_GE(sizeof(pagesize), sizeof(sysconf_result));
		
		void* mapping = ::mmap(nullptr, pagesize, PROT_READ, MAP_SHARED, fd.get(), 0);
		if (mapping != MAP_FAILED) {
			if (mprotect(mapping, pagesize, PROT_READ | PROT_EXEC) == 0) {
				result = true;
			}
			::munmap(mapping, pagesize);
		}
	}
	return result;
}

#endif  // defined(OS_LINUX) || defined(OS_AIX)

}	// namespace annety

