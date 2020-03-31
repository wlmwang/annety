// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 05 2019

#include "files/FileEnumerator.h"
#include "Logging.h"

#include <utility>		// std::move
#include <dirent.h>		// DIR,dirent,opendir
#include <errno.h>		// errno
#include <fnmatch.h>	// fnmatch
#include <string.h>		// memset
#include <sys/stat.h>

namespace annety
{
namespace
{
void get_stat(const FilePath& path, bool show_links, struct stat* st)
{
	DCHECK(st);
	const int res = show_links ? ::lstat(path.value().c_str(), st)
							   : ::stat(path.value().c_str(), st);
	if (res < 0) {
		// Print the stat() error message unless it was ENOENT and we're following
		// symlinks.
		if (!(errno == ENOENT && !show_links)) {
			DPLOG(ERROR) << "Couldn't stat" << path.value();
		}
		::memset(st, 0, sizeof(*st));
	}
}

}  // namespace anonymous

// FileEnumerator::FileInfo ----------------------------------------------------

FileEnumerator::FileInfo::FileInfo()
{
	::memset(&stat_, 0, sizeof(stat_));
}

FileEnumerator::FileInfo::~FileInfo() = default;

bool FileEnumerator::FileInfo::is_directory() const
{
	return S_ISDIR(stat_.st_mode);
}

FilePath FileEnumerator::FileInfo::get_name() const
{
	return filename_;
}

int64_t FileEnumerator::FileInfo::get_size() const
{
	return stat_.st_size;
}

TimeStamp FileEnumerator::FileInfo::get_last_modified_time() const
{
	return TimeStamp::from_time_t(stat_.st_mtime);
}

// FileEnumerator --------------------------------------------------------------

FileEnumerator::FileEnumerator(const FilePath& root_path,
							   bool recursive,
							   int file_type)
	: FileEnumerator(root_path,
					 recursive,
					 file_type,
					 FilePath::StringType(),
					 FolderSearchPolicy::MATCH_ONLY) {}

FileEnumerator::FileEnumerator(const FilePath& root_path,
							   bool recursive,
							   int file_type,
							   const FilePath::StringType& pattern)
	: FileEnumerator(root_path,
					 recursive,
					 file_type,
					 pattern,
					 FolderSearchPolicy::MATCH_ONLY) {}

FileEnumerator::FileEnumerator(const FilePath& root_path,
							   bool recursive,
							   int file_type,
							   const FilePath::StringType& pattern,
							   FolderSearchPolicy folder_search_policy)
	: current_directory_entry_(0)
	, root_path_(root_path)
	, recursive_(recursive)
	, file_type_(file_type)
	, pattern_(pattern)
	, folder_search_policy_(folder_search_policy)
{
	// INCLUDE_DOT_DOT must not be specified if recursive.
	DCHECK(!(recursive && (INCLUDE_DOT_DOT & file_type_)));

	if (recursive && !(file_type & SHOW_SYM_LINKS)) {
		struct stat st;
		get_stat(root_path, false, &st);
		visited_directories_.insert(st.st_ino);
	}

	pending_paths_.push(root_path);
}

FileEnumerator::~FileEnumerator() = default;

FilePath FileEnumerator::next()
{
	++current_directory_entry_;

	// While we've exhausted the entries in the current directory, do the next
	while (current_directory_entry_ >= directory_entries_.size()) {
		if (pending_paths_.empty()) {
			return FilePath();
		}

		root_path_ = pending_paths_.top();
		root_path_ = root_path_.strip_trailing_separators();
		pending_paths_.pop();

		DIR* dir = ::opendir(root_path_.value().c_str());
		if (!dir) {
			continue;
		}

		directory_entries_.clear();

		current_directory_entry_ = 0;
		struct dirent* dent;
		while ((dent = ::readdir(dir))) {
			FileInfo info;
			info.filename_ = FilePath(dent->d_name);

			if (should_skip(info.filename_)) {
				continue;
			}

			const bool is_pattern_match = is_pattern_matched(info.filename_);

			// MATCH_ONLY policy enumerates files and directories which matching
			// pattern only. So we can early skip further checks.
			if (folder_search_policy_ == FolderSearchPolicy::MATCH_ONLY && 
				!is_pattern_match)
			{
				continue;
			}

			// Do not call OS stat/lstat if there is no sense to do it. If pattern is
			// not matched (file will not appear in results) and search is not
			// recursive (possible directory will not be added to pending paths) -
			// there is no sense to obtain item below.
			if (!recursive_ && !is_pattern_match) {
				continue;
			}

			const FilePath full_path = root_path_.append(info.filename_);
			const bool show_sym_links = file_type_ & SHOW_SYM_LINKS;
			get_stat(full_path, show_sym_links, &info.stat_);

			const bool is_dir = info.is_directory();

			// Recursive mode: schedule traversal of a directory if either
			// SHOW_SYM_LINKS is on or we haven't visited the directory yet.
			if (recursive_ && is_dir && 
				(show_sym_links || visited_directories_.insert(info.stat_.st_ino).second))
			{
				pending_paths_.push(full_path);
			}

			if (is_pattern_match && is_type_matched(is_dir)) {
				directory_entries_.push_back(std::move(info));
			}
		}
		::closedir(dir);

		// MATCH_ONLY policy enumerates files in matched subfolders by "*" pattern.
		// ALL policy enumerates files in all subfolders by origin pattern.
		if (folder_search_policy_ == FolderSearchPolicy::MATCH_ONLY) {
			pattern_.clear();
		}
	}

	return root_path_.append(
		directory_entries_[current_directory_entry_].filename_);
}

FileEnumerator::FileInfo FileEnumerator::get_info() const
{
	return directory_entries_[current_directory_entry_];
}

bool FileEnumerator::is_pattern_matched(const FilePath& path) const
{
	return pattern_.empty() ||
		!::fnmatch(pattern_.c_str(), path.value().c_str(), FNM_NOESCAPE);
}

bool FileEnumerator::should_skip(const FilePath& path)
{
	FilePath::StringType base = path.basename().value();
	return base == "." || 
		   (base == ".." && !(INCLUDE_DOT_DOT & file_type_));
}

bool FileEnumerator::is_type_matched(bool is_dir) const
{
	return (file_type_ &
		(is_dir ? FileEnumerator::DIRECTORIES : FileEnumerator::FILES)) != 0;
}

}	// namespace annety

