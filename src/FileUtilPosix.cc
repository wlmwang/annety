// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 05 2019

#include "FileUtil.h"
#include "FilePath.h"
#include "FileEnumerator.h"
#include "ScopedFile.h"
#include "EintrWrapper.h"
#include "StringUtil.h"

#include "BuildConfig.h"
#include "Macros.h"
#include "Logging.h"
#include "Time.h"

#if defined(OS_MACOSX)
#include <copyfile.h>
#include <AvailabilityMacros.h>
#endif

#include <errno.h>		// errno
#include <grp.h>		// getgrnam
#include <fcntl.h>		// fcntl,O_NONBLOCK
#include <stdio.h>		// rename,access,fopen
#include <limits.h>		// PATH_MAX
#include <stdlib.h>		// mkstemp,realpath
#include <sys/stat.h>	// stat,S_ISLNK
#include <sys/types.h>	// size_t
#include <unistd.h>		// mkdir,rmdir,unlink,pipe,readlink,close
#include <stddef.h>

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

std::string temp_file_name() {
	return std::string(kTempFileNamePattern);
}

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

	// !ContainsKey(group_gids, stat_info.st_gid)
	if ((stat_info.st_mode & S_IWGRP) &&
		group_gids.find(stat_info.st_gid) == group_gids.end())
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

// Helper for do_copy_directory
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

// Helper for copy_directory
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
		real_to_path = make_absolute_filepath(real_to_path.dirname());
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
		from_path_base = from_path.dirname();
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

}	// namespace anonymous

// -------------------------------------------------------------------

#if defined(OS_MACOSX)
bool get_tempdir(FilePath* path) {
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

FilePath get_homedir() {
	const char* home_dir = ::getenv("HOME");
	if (home_dir && home_dir[0]) {
		return FilePath(home_dir);
	}

	// Fall back on temp dir if no home directory is defined.
	FilePath rv;
	if (get_tempdir(&rv)) {
		return rv;
	}

	// Last resort.
	return FilePath("/tmp");
}
#else
bool get_tempdir(FilePath* path) {
	const char* tmp = ::getenv("TMPDIR");
	if (tmp) {
		*path = FilePath(tmp);
		return true;
	}

	*path = FilePath("/tmp");
	return true;
}

FilePath get_homedir() {
	const char* home_dir = ::getenv("HOME");
	if (home_dir && home_dir[0]) {
		return FilePath(home_dir);
	}

	FilePath rv;
	if (get_tempdir(&rv)) {
		return rv;
	}

	// Last resort.
	return FilePath("/tmp");
}
#endif  // defined(OS_MACOSX)

bool copy_directory(const FilePath& from_path,
					const FilePath& to_path,
					bool recursive)
{
	return do_copy_directory(from_path, to_path, recursive, false);
}

bool copy_directory_excl(const FilePath& from_path,
						 const FilePath& to_path,
						 bool recursive)
{
	return do_copy_directory(from_path, to_path, recursive, true);
}

bool directory_exists(const FilePath& path) {
	stat_wrapper_t file_info;
	if (call_stat(path.value().c_str(), &file_info) != 0) {
		return false;
	}
	return S_ISDIR(file_info.st_mode);
}

bool path_exists(const FilePath& path) {
	return ::access(path.value().c_str(), F_OK) == 0;
}

bool path_is_writable(const FilePath& path) {
	return ::access(path.value().c_str(), W_OK) == 0;
}

FilePath make_absolute_filepath(const FilePath& input) {
	char full_path[PATH_MAX];
	if (::realpath(input.value().c_str(), full_path) == nullptr) {
		return FilePath();
	}
	return FilePath(full_path);
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

bool get_file_size(const FilePath& file_path, int64_t* file_size) {
	File::Info info;
	if (!get_file_info(file_path, &info)) {
		DLOG(ERROR) << "Unable to get file info " << file_path.value();
		return false;
	}
	*file_size = info.size;
	return true;
}

bool get_file_info(const FilePath& file_path, File::Info* results) {
	stat_wrapper_t file_info;
	if (call_stat(file_path.value().c_str(), &file_info) != 0) {
		return false;
	}

	results->from_stat(file_info);
	return true;
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

bool touch_file(const FilePath& path,
				const Time& last_accessed,
				const Time& last_modified)
{
	int flags = File::FLAG_OPEN;

	File file(path, flags);
	if (!file.is_valid()) {
		DLOG(ERROR) << "The filepath is invalid " << path.value();
		return false;
	}
	return file.set_times(last_accessed, last_modified);
}

bool move_file(const FilePath& from_path, const FilePath& to_path) {
	if (from_path.references_parent() || to_path.references_parent()) {
		DLOG(ERROR) << "Unable to move_file reference parent path " 
					<< from_path.value() << ">>>" << to_path.value();
		return false;
	}
	return internal::move_unsafe(from_path, to_path);
}

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

bool replace_file(const FilePath& from_path, const FilePath& to_path, File::Error* error) {
	if (::rename(from_path.value().c_str(), to_path.value().c_str()) == 0) {
		return true;
	}
	if (error) {
		*error = File::get_last_file_error();
	}
	return false;
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

bool set_posix_file_permissions(const FilePath& path,
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
	ssize_t count = ::readlink(symlink_path.value().c_str(), buf, arraysize(buf));

	if (count <= 0) {
		target_path->clear();
		return false;
	}

	*target_path = FilePath(FilePath::StringType(buf, count));
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

bool create_temporary_file(FilePath* path) {
	// For call to close().
	FilePath directory;
	if (!get_tempdir(&directory)) {
		return false;
	}

	int fd = create_and_open_fd_for_temporary_file_in_dir(directory, path);
	if (fd < 0) {
		return false;
	}
	
	::close(fd);
	return true;
}

bool create_temporary_file_in_dir(const FilePath& dir, FilePath* temp_file) {
	// For call to close().
	int fd = create_and_open_fd_for_temporary_file_in_dir(dir, temp_file);
	return ((fd >= 0) && !IGNORE_EINTR(::close(fd)));
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

FILE* create_and_open_temporary_file(FilePath* path) {
	FilePath directory;
	if (!get_tempdir(&directory)) {
		DLOG(ERROR) << "Unable to get temp dir";
		return nullptr;
	}
	return create_and_open_temporary_file_in_dir(directory, path);
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
	if (!get_tempdir(&tmpdir)) {
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
	for (FilePath path = full_path.dirname();
		path.value() != last_path.value(); path = path.dirname())
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
		if (::mkdir(i->value().c_str(), 0700) == 0) {
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

int get_maximum_path_component_length(const FilePath& path) {
	return ::pathconf(path.value().c_str(), _PC_NAME_MAX);
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

}	// namespace annety

