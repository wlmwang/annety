// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions for dealing with the local
// filesystem.

// Refactoring: Anny Wang
// Date: Jun 05 2019

#ifndef ANT_FILE_UTIL_H_
#define ANT_FILE_UTIL_H_

#include "BuildConfig.h"
#include "StringPiece.h"
#include "File.h"
#include "FilePath.h"

#include <stddef.h>		// size_t
#include <inttypes.h>	// gid_t,uid_t,int64_t
#include <stdio.h>

#include <set>
#include <stack>
#include <string>

namespace annety
{
class Time;

namespace files
{
//-----------------------------------------------------------------------------
// Functions that involve filesystem access or modification:

// Returns an absolute version of a relative path. Returns an empty path on
// error. On POSIX, this function fails if the path does not exist. This
// function can result in I/O so it can be slow.
FilePath make_absolute_filepath(const FilePath& input);

// Sets |real_path| to |path| with symbolic links and junctions expanded.
// On windows, make sure the path starts with a lettered drive.
// |path| must reference a file.  Function will fail if |path| points to
// a directory or to a nonexistent path.  On windows, this function will
// fail if |path| is a junction or symlink that points to an empty file,
// or if |real_path| would be longer than MAX_PATH characters.
bool normalize_filepath(const FilePath& path, FilePath* real_path);

// Returns the total number of bytes used by all the files under |root_path|.
// If the path does not exist the function returns 0.
//
// This function is implemented using the FileEnumerator class so it is not
// particularly speedy in any platform.
int64_t compute_directory_size(const FilePath& root_path);

// Sets the time of the last access and the time of the last modification.
bool touch_file(const FilePath& path, const Time& last_accessed, const Time& last_modified);

// Moves the given path, whether it's a file or a directory.
// If a simple rename is not possible, such as in the case where the paths are
// on different volumes, this will attempt to copy and delete. Returns
// true for success.
// This function fails if either path contains traversal components ('..').
bool move_file(const FilePath& from_path, const FilePath& to_path);

// Deletes the given path, whether it's a file or a directory.
// If it's a directory, it's perfectly happy to delete all of the
// directory's contents.  Passing true to recursive deletes
// subdirectories and their contents as well.
// Returns true if successful, false otherwise. It is considered successful
// to attempt to delete a file that does not exist.
//
// In posix environment and if |path| is a symbolic link, this deletes only
// the symlink. (even if the symlink points to a non-existent file)
//
// WARNING: USING THIS WITH recursive==true IS EQUIVALENT
//          TO "rm -rf", SO USE WITH CAUTION.
bool delete_file(const FilePath& path, bool recursive);

// Renames file |from_path| to |to_path|. Both paths must be on the same
// volume, or the function will fail. Destination file will be created
// if it doesn't exist. Prefer this function over Move when dealing with
// temporary files. On Windows it preserves attributes of the target file.
// Returns true on success, leaving *error unchanged.
// Returns false on failure and sets *error appropriately, if it is non-NULL.
bool replace_file(const FilePath& from_path, const FilePath& to_path, 
				  File::Error* error);

// Copies a single file. Use CopyDirectory() to copy directories.
// This function fails if either path contains traversal components ('..').
// This function also fails if |to_path| is a directory.
//
// On POSIX, if |to_path| is a symlink, CopyFile() will follow the symlink. This
// may have security implications. Use with care.
//
// If |to_path| already exists and is a regular file, it will be overwritten,
// though its permissions will stay the same.
//
// If |to_path| does not exist, it will be created. The new file's permissions
// varies per platform:
//
// - This function keeps the metadata on Windows. The read only bit is not kept.
// - On Mac and iOS, |to_path| retains |from_path|'s permissions, except user
//   read/write permissions are always set.
// - On Linux and Android, |to_path| has user read/write permissions only. i.e.
//   Always 0600.
// - On ChromeOS, |to_path| has user read/write permissions and group/others
//   read permissions. i.e. Always 0644.
bool copy_file(const FilePath& from_path, const FilePath& to_path);

// Copies the given path, and optionally all subdirectories and their contents
// as well.
//
// If there are files existing under to_path, always overwrite. Returns true
// if successful, false otherwise. Wildcards on the names are not supported.
//
// This function has the same metadata behavior as CopyFile().
//
// If you only need to copy a file use CopyFile, it's faster.
bool copy_directory(const FilePath& from_path,
					const FilePath& to_path,
					bool recursive);

// Like CopyDirectory() except trying to overwrite an existing file will not
// work and will return false.
bool copy_directory_excl(const FilePath& from_path,
						 const FilePath& to_path,
						 bool recursive);

// Returns true if the given path exists on the local filesystem,
// false otherwise.
bool path_exists(const FilePath& path);

// Returns true if the given path is writable by the user, false otherwise.
bool path_is_writable(const FilePath& path);

// Returns true if the given path exists and is a directory, false otherwise.
bool directory_exists(const FilePath& path);

// Performs the same function as CreateAndOpenTemporaryFileInDir(), but returns
// the file-descriptor directly, rather than wrapping it into a FILE. Returns
// -1 on failure.
int create_and_open_fd_for_temporary_file_in_dir(const FilePath& dir,
												 FilePath* path);

// Bits and masks of the file permission.
enum FilePermissionBits
{
	FILE_PERMISSION_MASK              = S_IRWXU | S_IRWXG | S_IRWXO,
	FILE_PERMISSION_USER_MASK         = S_IRWXU,
	FILE_PERMISSION_GROUP_MASK        = S_IRWXG,
	FILE_PERMISSION_OTHERS_MASK       = S_IRWXO,

	FILE_PERMISSION_READ_BY_USER      = S_IRUSR,
	FILE_PERMISSION_WRITE_BY_USER     = S_IWUSR,
	FILE_PERMISSION_EXECUTE_BY_USER   = S_IXUSR,
	FILE_PERMISSION_READ_BY_GROUP     = S_IRGRP,
	FILE_PERMISSION_WRITE_BY_GROUP    = S_IWGRP,
	FILE_PERMISSION_EXECUTE_BY_GROUP  = S_IXGRP,
	FILE_PERMISSION_READ_BY_OTHERS    = S_IROTH,
	FILE_PERMISSION_WRITE_BY_OTHERS   = S_IWOTH,
	FILE_PERMISSION_EXECUTE_BY_OTHERS = S_IXOTH,
};

// Reads the permission of the given |path|, storing the file permission
// bits in |mode|. If |path| is symbolic link, |mode| is the permission of
// a file which the symlink points to.
bool get_posix_file_permissions(const FilePath& path, int* mode);
// Sets the permission of the given |path|. If |path| is symbolic link, sets
// the permission of a file which the symlink points to.
bool set_posix_file_permissions(const FilePath& path, int mode);

// Get the home directory. This is more complicated than just getenv("HOME")
// as it knows to fall back on getpwent() etc.
//
// You should not generally call this directly. Instead use DIR_HOME with the
// path service which will use this function but cache the value.
// Path service may also override DIR_HOME.
FilePath get_homedir();

// Get the temporary directory provided by the system.
//
// WARNING: In general, you should use CreateTemporaryFile variants below
// instead of this function. Those variants will ensure that the proper
// permissions are set so that other users on the system can't edit them while
// they're open (which can lead to security issues).
bool get_tempdir(FilePath* path);

// Creates a temporary file. The full path is placed in |path|, and the
// function returns true if was successful in creating the file. The file will
// be empty and all handles closed after this function returns.
bool create_temporary_file(FilePath* path);

// Same as CreateTemporaryFile but the file is created in |dir|.
bool create_temporary_file_in_dir(const FilePath& dir,
								  FilePath* temp_file);

// Create and open a temporary file.  File is opened for read/write.
// The full path is placed in |path|.
// Returns a handle to the opened file or NULL if an error occurred.
FILE* create_and_open_temporary_file(FilePath* path);

// Similar to CreateAndOpenTemporaryFile, but the file is created in |dir|.
FILE* create_and_open_temporary_file_in_dir(const FilePath& dir,
											FilePath* path);

// Create a new directory. If prefix is provided, the new directory name is in
// the format of prefixyyyy.
// NOTE: prefix is ignored in the POSIX implementation.
// If success, return true and output the full path of the directory created.
bool create_new_temp_directory(const FilePath::StringType& prefix,
								FilePath* new_temp_path);

// Create a directory within another directory.
// Extra characters will be appended to |prefix| to ensure that the
// new directory does not have the same name as an existing directory.
bool create_temporary_dir_in_dir(const FilePath& base_dir,
								const FilePath::StringType& prefix,
								FilePath* new_dir);

// Creates a directory, as well as creating any parent directories, if they
// don't exist. Returns 'true' on successful creation, or if the directory
// already exists.  The directory is only readable by the current user.
// Returns true on success, leaving *error unchanged.
// Returns false on failure and sets *error appropriately, if it is non-NULL.
bool create_directory_and_get_error(const FilePath& full_path,
									File::Error* error);

// Backward-compatible convenience method for the above.
bool create_directory(const FilePath& full_path);

// Returns true if the given directory is empty
bool is_directory_empty(const FilePath& dir_path);

// Gets the current working directory for the process.
bool get_current_directory(FilePath* path);

// Sets the current working directory for the process.
bool set_current_directory(const FilePath& path);

// Creates a symbolic link at |symlink| pointing to |target|.  Returns
// false on failure.
bool create_symbolic_link(const FilePath& target,
						  const FilePath& symlink);

// Reads the given |symlink| and returns where it points to in |target|.
// Returns false upon failure.
bool read_symbolic_link(const FilePath& symlink, FilePath* target);

// This function will return if the given file is a symlink or not.
bool is_link(const FilePath& file_path);

// Returns the file size. Returns true on success.
bool get_file_size(const FilePath& file_path, int64_t* file_size);

// Returns information about the given file path.
bool get_file_info(const FilePath& file_path, File::Info* info);

// Associates a standard FILE stream with an existing File. Note that this
// functions take ownership of the existing File.
FILE* file_to_FILE(File file, const char* mode);

// Wrapper for fopen-like calls. Returns non-NULL FILE* on success. The
// underlying file descriptor (POSIX) or handle (Windows) is unconditionally
// configured to not be propagated to child processes.
FILE* open_FILE(const FilePath& filename, const char* mode);

// Closes file opened by OpenFile. Returns true on success.
bool close_FILE(FILE* file);

// Truncates an open file to end at the location of the current file pointer.
// This is a cross-platform analog to Windows' SetEndOfFile() function.
bool truncate_FILE(FILE* file);

// Returns true if the contents of the two files given are equal, false
// otherwise.  If either file can't be read, returns false.
bool contents_equal(const FilePath& filename1,
					const FilePath& filename2);

// Returns true if the contents of the two text files given are equal, false
// otherwise.  This routine treats "\r\n" and "\n" as equivalent.
bool text_contents_equal(const FilePath& filename1,
						 const FilePath& filename2);


// Reads the file at |path| into |contents| and returns true on success and
// false on error.  For security reasons, a |path| containing path traversal
// components ('..') is treated as a read error and |contents| is set to empty.
// In case of I/O error, |contents| holds the data that could be read from the
// file before the error occurred.  When the file size exceeds |max_size|, the
// function returns false with |contents| holding the file truncated to
// |max_size|.
// |contents| may be NULL, in which case this function is useful for its side
// effect of priming the disk cache (could be used for unit tests).
bool read_file_to_string_with_max_size(const FilePath& path,
									   std::string* contents,
									   size_t max_size);

// Reads the file at |path| into |contents| and returns true on success and
// false on error.  For security reasons, a |path| containing path traversal
// components ('..') is treated as a read error and |contents| is set to empty.
// In case of I/O error, |contents| holds the data that could be read from the
// file before the error occurred.
// |contents| may be NULL, in which case this function is useful for its side
// effect of priming the disk cache (could be used for unit tests).
bool read_file_to_string(const FilePath& path, std::string* contents);

// Read exactly |bytes| bytes from file descriptor |fd|, storing the result
// in |buffer|. This function is protected against EINTR and partial reads.
// Returns true iff |bytes| bytes have been successfully read from |fd|.
bool read_from_fd(int fd, char* buffer, size_t bytes);

// Reads at most the given number of bytes from the file into the buffer.
// Returns the number of read bytes, or -1 on error.
int read_file(const FilePath& filename, char* data, int max_size);

// Writes the given buffer into the file, overwriting any data that was
// previously there.  Returns the number of bytes written, or -1 on error.
int write_file(const FilePath& filename, const char* data, int size);

// Appends |data| to |fd|. Does not close |fd| when done.  Returns true iff
// |size| bytes of |data| were written to |fd|.
bool write_to_fd(const int fd, const char* data, int size);

// Appends |data| to |filename|.  Returns true iff |size| bytes of |data| were
// written to |filename|.
bool append_to_file(const FilePath& filename, const char* data, int size);

// Sets the given |fd| to non-blocking mode.
// Returns true if it was able to set it in the non-blocking mode, otherwise
// false.
bool set_non_blocking(int fd);

// Sets the given |fd| to close-on-exec mode.
// Returns true if it was able to set it in the close-on-exec mode, otherwise
// false.
bool set_close_on_exec(int fd);

// Creates a non-blocking, close-on-exec pipe.
// This creates a non-blocking pipe that is not intended to be shared with any
// child process. This will be done atomically if the operating system supports
// it. Returns true if it was able to create the pipe, otherwise false.
bool create_local_non_blocking_pipe(int fds[2]);

// Test that |path| can only be changed by a given user and members of
// a given set of groups.
// Specifically, test that all parts of |path| under (and including) |base|:
// * Exist.
// * Are owned by a specific user.
// * Are not writable by all users.
// * Are owned by a member of a given set of groups, or are not writable by
//   their group.
// * Are not symbolic links.
// This is useful for checking that a config file is administrator-controlled.
// |base| must contain |path|.
bool verify_path_controlled_by_user(const FilePath& base,
									const FilePath& path,
									uid_t owner_uid,
									const std::set<gid_t>& group_gids);

// Attempts to find a number that can be appended to the |path| to make it
// unique. If |path| does not exist, 0 is returned.  If it fails to find such
// a number, -1 is returned. If |suffix| is not empty, also checks the
// existence of it with the given suffix.
int get_unique_path_number(const FilePath& path,
						   const FilePath::StringType& suffix);

// Returns the maximum length of path component on the volume containing
// the directory |path|, in the number of FilePath::CharType, or -1 on failure.
int get_maximum_path_component_length(const FilePath& path);

// Internal --------------------------------------------------------------------
namespace internal
{

// Same as Move but allows paths with traversal components.
// Use only with extreme care.
bool move_unsafe(const FilePath& from_path,
				 const FilePath& to_path);

}  // namespace internal

}	// namespace files
}	// namespace annety

#endif	// ANT_FILE_UTIL_H_
