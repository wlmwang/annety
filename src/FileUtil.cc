// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utility functions for dealing with the local
// filesystem.

// Modify: Anny Wang
// Date: Jun 05 2019

#include "FileUtil.h"
#include "FilePath.h"
#include "FileEnumerator.h"
#include "StringPiece.h"
#include "StringPrintf.h"
#include "StringUtil.h"
#include "Logging.h"

#include <fstream>
#include <limits>
#include <stdio.h>

namespace annety {
namespace {
// The maximum number of 'uniquified' files we will try to create.
// This is used when the filename we're trying to download is already in use,
// so we create a new unique filename by appending " (nnn)" before the
// extension, where 1 <= nnn <= kMaxUniqueFiles.
// Also used by code that cleans up said files.
static const int kMaxUniqueFiles = 100;

}	// namespace anonymous

int64_t compute_directory_size(const FilePath& root_path) {
	int64_t running_size = 0;
	FileEnumerator file_iter(root_path, true, FileEnumerator::FILES);
	while (!file_iter.next().empty()) {
		running_size += file_iter.get_info().get_size();
	}
	return running_size;
}

bool move(const FilePath& from_path, const FilePath& to_path) {
	if (from_path.references_parent() || to_path.references_parent()) {
		return false;
	}
	return internal::move_unsafe(from_path, to_path);
}

bool contents_equal(const FilePath& filename1, const FilePath& filename2) {
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
			(memcmp(buffer1, buffer2, static_cast<size_t>(file1.gcount()))))
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

bool text_contents_equal(const FilePath& filename1, const FilePath& filename2) {
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
		return false;
	}
	
	FILE* file = open_file(path, "rb");
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
	close_file(file);
	if (contents) {
		contents->swap(local_contents);
		contents->resize(bytes_read_so_far);
	}
	return read_status;
}

bool read_file_to_string(const FilePath& path, std::string* contents) {
	return read_file_to_string_with_max_size(path, contents,
				std::numeric_limits<size_t>::max());
}

bool is_directory_empty(const FilePath& dir_path) {
	FileEnumerator files(dir_path, false,
						FileEnumerator::FILES | FileEnumerator::DIRECTORIES);
	if (files.next().empty()) {
		return true;
	}
	return false;
}

FILE* create_and_open_temporary_file(FilePath* path) {
	FilePath directory;
	if (!get_temp_dir(&directory)) {
		return nullptr;
	}
	return create_and_open_temporary_file_in_dir(directory, path);
}

bool create_directory(const FilePath& full_path) {
	return create_directory_and_get_error(full_path, nullptr);
}

bool get_file_size(const FilePath& file_path, int64_t* file_size) {
	File::Info info;
	if (!get_file_info(file_path, &info)) {
		return false;
	}
	*file_size = info.size;
	return true;
}

bool touch_file(const FilePath& path,
				const Time& last_accessed,
				const Time& last_modified)
{
	int flags = File::FLAG_OPEN;

	File file(path, flags);
	if (!file.is_valid()) {
		return false;
	}
	return file.set_times(last_accessed, last_modified);
}

bool close_file(FILE* file) {
	if (file == nullptr) {
		return true;
	}
	return ::fclose(file) == 0;
}

bool truncate_file(FILE* file) {
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
