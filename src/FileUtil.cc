// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions for dealing with the local
// filesystem.

// By: wlmwang
// Date: Jun 05 2019

#include "files/FileUtil.h"
#include "files/FilePath.h"
#include "files/FileEnumerator.h"
#include "strings/StringPiece.h"
#include "strings/StringPrintf.h"
#include "EintrWrapper.h"
#include "Logging.h"

#include <algorithm>	// std::min
#include <fstream>		// ifstream
#include <fcntl.h>		// fcntl,O_NONBLOCK
#include <limits>		// numeric_limits<size_t>
#include <string>		// std::string,std::getline
#include <string.h>		// memcmp
#include <stdio.h>		// fread,feof,ferror,ftell,fileno
#include <stddef.h>

namespace annety
{
namespace
{
// The maximum number of 'uniquified' files we will try to create.
// This is used when the filename we're trying to download is already in use,
// so we create a new unique filename by appending " (nnn)" before the
// extension, where 1 <= nnn <= kMaxUniqueFiles.
// Also used by code that cleans up said files.
static const int kMaxUniqueFiles = 100;

#if !defined(OS_MACOSX)
// Appends |mode_char| to |mode| before the optional character set encoding; see
// https://www.gnu.org/software/libc/manual/html_node/Opening-Streams.html for
// details.
std::string append_mode_character(StringPiece mode, char mode_char)
{
	std::string result(mode.as_string());
	size_t comma_pos = result.find(',');
	result.insert(comma_pos == std::string::npos ? result.length() : comma_pos, 1,
				  mode_char);
	return result;
}
#endif

}	// namespace anonymous

bool is_directory_empty(const FilePath& dir_path)
{
	FileEnumerator files(dir_path, false,
						 FileEnumerator::FILES | FileEnumerator::DIRECTORIES);
	if (files.next().empty()) {
		return true;
	}
	return false;
}

int64_t compute_directory_size(const FilePath& root_path)
{
	int64_t running_size = 0;
	FileEnumerator file_iter(root_path, true, FileEnumerator::FILES);
	while (!file_iter.next().empty()) {
		running_size += file_iter.get_info().get_size();
	}
	return running_size;
}

bool create_directory(const FilePath& full_path)
{
	return create_directory_and_get_error(full_path, nullptr);
}

FILE* file_to_FILE(File file, const char* mode)
{
	FILE* stream = ::fdopen(file.get_platform_file(), mode);
	if (stream) {
		file.take_platform_file();
	}
	return stream;
}

FILE* open_FILE(const FilePath& filename, const char* mode)
{
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

bool close_FILE(FILE* file)
{
	if (file == nullptr) {
		return true;
	}
	return ::fclose(file) == 0;
}

bool truncate_FILE(FILE* file)
{
	if (file == nullptr) {
		return false;
	}

	long current_offset = ::ftell(file);
	if (current_offset == -1) {
		return false;
	}

	int fd = ::fileno(file);
	if (::ftruncate(fd, current_offset) != 0) {
		return false;
	}
	return true;
}

bool contents_equal(const FilePath& filename1, const FilePath& filename2)
{
	// We open the file in binary format even if they are text files because
	// we are just comparing that bytes are exactly same in both files and not
	// doing anything smart with text formatting.
	std::ifstream file1(filename1.value().c_str(),
						std::ios::in | std::ios::binary);
	std::ifstream file2(filename2.value().c_str(),
						std::ios::in | std::ios::binary);

	// Even if both files aren't openable (and thus, in some sense, "equal"),
	// any unusable file yields a result of "false".
	if (!file1.is_open() || !file2.is_open()) {
		return false;
	}

	const int BUFFER_SIZE = 2056;
	char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
	do {
		file1.read(buffer1, BUFFER_SIZE);
		file2.read(buffer2, BUFFER_SIZE);

		if ((file1.eof() != file2.eof()) ||
			(file1.gcount() != file2.gcount()) ||
			(::memcmp(buffer1, buffer2, static_cast<size_t>(file1.gcount()))))
		{
			file1.close();
			file2.close();
			return false;
		}
	} while (!file1.eof() || !file2.eof());

	file1.close();
	file2.close();
	return true;
}

bool text_contents_equal(const FilePath& filename1, const FilePath& filename2)
{
	std::ifstream file1(filename1.value().c_str(), std::ios::in);
	std::ifstream file2(filename2.value().c_str(), std::ios::in);

	// Even if both files aren't openable (and thus, in some sense, "equal"),
	// any unusable file yields a result of "false".
	if (!file1.is_open() || !file2.is_open()) {
		return false;
	}

	do {
		std::string line1, line2;
		std::getline(file1, line1);
		std::getline(file2, line2);

		// Check for mismatched EOF states, or any error state.
		if ((file1.eof() != file2.eof()) ||
			file1.bad() || file2.bad())
		{
			return false;
		}

		// Trim all '\r' and '\n' characters from the end of the line.
		std::string::size_type end1 = line1.find_last_not_of("\r\n");
		if (end1 == std::string::npos) {
			line1.clear();
		} else if (end1 + 1 < line1.length()) {
			line1.erase(end1 + 1);
		}

		std::string::size_type end2 = line2.find_last_not_of("\r\n");
		if (end2 == std::string::npos) {
			line2.clear();
		} else if (end2 + 1 < line2.length()) {
			line2.erase(end2 + 1);
		}

		if (line1 != line2) {
			return false;
		}
	} while (!file1.eof() || !file2.eof());

	return true;
}

bool read_file_to_string_with_max_size(const FilePath& path,
									   std::string* contents,
									   size_t max_size)
{
	if (contents) {
		contents->clear();
	}
	if (path.references_parent()) {
		DLOG(ERROR) << "Unable to read reference parent file " << path.value();
		return false;
	}
	
	FILE* file = open_FILE(path, "rb");
	if (!file) {
		return false;
	}

	// Many files supplied in |path| have incorrect size (proc files etc).
	// Hence, the file is read sequentially as opposed to a one-shot read, using
	// file size as a hint for chunk size if available.
	constexpr int64_t kDefaultChunkSize = 1 << 16;
	int64_t chunk_size;
	if (!get_file_size(path, &chunk_size) || chunk_size <= 0) {
		chunk_size = kDefaultChunkSize - 1;
	}
	// We need to attempt to read at EOF for feof flag to be set so here we
	// use |chunk_size| + 1.
	chunk_size = std::min<uint64_t>(chunk_size, max_size) + 1;
	size_t bytes_read_this_pass;
	size_t bytes_read_so_far = 0;
	bool read_status = true;
	std::string local_contents;
	local_contents.resize(chunk_size);

	while ((bytes_read_this_pass = ::fread(&local_contents[bytes_read_so_far], 1,
										 chunk_size, file)) > 0)
	{
		if ((max_size - bytes_read_so_far) < bytes_read_this_pass) {
			// Read more than max_size bytes, bail out.
			bytes_read_so_far = max_size;
			read_status = false;
			break;
		}
		// In case EOF was not reached, iterate again but revert to the default
		// chunk size.
		if (bytes_read_so_far == 0) {
			chunk_size = kDefaultChunkSize;
		}

		bytes_read_so_far += bytes_read_this_pass;
		// Last fread syscall (after EOF) can be avoided via feof, which is just a
		// flag check.
		if (::feof(file)) {
			break;
		}
		local_contents.resize(bytes_read_so_far + chunk_size);
	}

	read_status = read_status && !::ferror(file);
	close_FILE(file);
	if (contents) {
		contents->swap(local_contents);
		contents->resize(bytes_read_so_far);
	}
	return read_status;
}

bool read_file_to_string(const FilePath& path, std::string* contents)
{
	return read_file_to_string_with_max_size(path, contents,
				std::numeric_limits<size_t>::max());
}

bool read_from_fd(int fd, char* buffer, size_t bytes)
{
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

int read_file(const FilePath& filename, char* data, int max_size)
{
	int fd = HANDLE_EINTR(::open(filename.value().c_str(), O_RDONLY));
	if (fd < 0) {
		DPLOG(ERROR) << "Unable to open file " << filename.value();
		return -1;
	}

	ssize_t bytes_read = HANDLE_EINTR(::read(fd, data, max_size));
	if (IGNORE_EINTR(::close(fd)) < 0) {
		DPLOG(ERROR) << "Error while closing file " << filename.value();
		return -1;
	}
	return bytes_read;
}

bool write_to_fd(const int fd, const char* data, int size)
{
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

int write_file(const FilePath& filename, const char* data, int size)
{
	int fd = HANDLE_EINTR(::creat(filename.value().c_str(), 0666));
	if (fd < 0) {
		DPLOG(ERROR) << "Unable to creat file " << filename.value();
		return -1;
	}

	int bytes_written = write_to_fd(fd, data, size) ? size : -1;
	if (IGNORE_EINTR(::close(fd)) < 0) {
		DPLOG(ERROR) << "Error while closing file " << filename.value();
		return -1;
	}
	return bytes_written;
}

bool append_to_file(const FilePath& filename, const char* data, int size)
{
	bool ret = true;
	int fd = HANDLE_EINTR(::open(filename.value().c_str(), O_WRONLY | O_APPEND));
	if (fd < 0) {
		DPLOG(ERROR) << "Unable to open file " << filename.value();
		return false;
	}

	// This call will either write all of the data or return false.
	if (!write_to_fd(fd, data, size)) {
		DLOG(ERROR) << "Error while writing to file " << filename.value();
		ret = false;
	}

	if (IGNORE_EINTR(::close(fd)) < 0) {
		DPLOG(ERROR) << "Error while closing file " << filename.value();
		return false;
	}

	return ret;
}

bool set_non_blocking(int fd)
{
	const int flags = ::fcntl(fd, F_GETFL);
	if (flags == -1) {
		DPLOG(ERROR) << "Unable to fcntl file F_GETFL " << fd;
		return false;
	}
	if (flags & O_NONBLOCK) {
		return true;
	}
	if (HANDLE_EINTR(::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) == -1) {
		DPLOG(ERROR) << "Unable to fcntl file O_NONBLOCK";
		return false;
	}
	return true;
}

bool set_close_on_exec(int fd)
{
	const int flags = ::fcntl(fd, F_GETFD);
	if (flags == -1) {
		DPLOG(ERROR) << "Unable to fcntl file F_GETFD " << fd;
		return false;
	}
	if (flags & FD_CLOEXEC) {
		return true;
	}
	if (HANDLE_EINTR(::fcntl(fd, F_SETFD, flags | FD_CLOEXEC)) == -1) {
		DPLOG(ERROR) << "Unable to fcntl file FD_CLOEXEC " << fd;
		return false;
	}
	return true;
}

int get_unique_path_number(const FilePath& path,
						   const FilePath::StringType& suffix)
{
	bool have_suffix = !suffix.empty();
	if (!path_exists(path) &&
		(!have_suffix || !path_exists(FilePath(path.value() + suffix))))
	{
		return 0;
	}

	FilePath new_path;
	for (int count = 1; count <= kMaxUniqueFiles; ++count) {
		new_path = path.insert_before_extension(string_printf(" (%d)", count));
		if (!path_exists(new_path) &&
			(!have_suffix || !path_exists(FilePath(new_path.value() + suffix))))
		{
			return count;
		}
	}
	return -1;
}

}	// namespace annety
