// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 05 2019

#ifndef ANT_FILES_FILE_ENUMERATOR_H_
#define ANT_FILES_FILE_ENUMERATOR_H_

#include "Macros.h"
#include "TimeStamp.h"
#include "files/FilePath.h"

#include <vector>
#include <stack>
#include <unordered_set>
#include <sys/stat.h>		// stat
#include <sys/types.h>		// size_t,ino_t

namespace annety
{
// Example:
// // FileEnumerator
// cout << "scan all (*.cc) file in current directory:" << endl;
// FileEnumerator enums(FilePath("./"), false, FileEnumerator::FILES, "*.cc");
// for (FilePath name = enums.next(); !name.empty(); name = enums.next()) {
//		cout << name << endl;
// }
// ...

// A class for enumerating the files in a provided path. The order of the
// results is not guaranteed.
//
// Important: This is blocking call. Do not use it on critical threads.
// *Not thread safe*
//
// FIXME: Add a iterator for easy for-range.
class FileEnumerator
{
public:
	// Note: copy & assign supported.
	class FileInfo
	{
	public:
		FileInfo();
		~FileInfo();

		bool is_directory() const;

		// The name of the file. This will not include any path information. This
		// is in constrast to the value returned by FileEnumerator.Next() which
		// includes the |root_path| passed into the FileEnumerator constructor.
		FilePath get_name() const;

		int64_t get_size() const;
		TimeStamp get_last_modified_time() const;

		const struct stat& stat() const { return stat_;}

	private:
		friend class FileEnumerator;

		struct stat stat_;
		FilePath filename_;
	};

	enum FileType
	{
		FILES = 1 << 0,
		DIRECTORIES = 1 << 1,
		INCLUDE_DOT_DOT = 1 << 2,
		SHOW_SYM_LINKS = 1 << 4,
	};

	// Search policy for intermediate folders.
	enum class FolderSearchPolicy
	{
		// Recursive search will pass through folders whose names match the
		// pattern. Inside each one, all files will be returned. Folders with names
		// that do not match the pattern will be ignored within their interior.
		MATCH_ONLY,
		// Recursive search will pass through every folder and perform pattern
		// matching inside each one.
		ALL,
	};

	// |root_path| is the starting directory to search for. It may or may not end
	// in a slash.
	//
	// If |recursive| is true, this will enumerate all matches in any
	// subdirectories matched as well. It does a breadth-first search, so all
	// files in one directory will be returned before any files in a
	// subdirectory.
	//
	// |file_type|, a bit mask of FileType, specifies whether the enumerator
	// should match files, directories, or both.
	//
	// |pattern| is an optional pattern for which files to match. This
	// works like shell globbing. For example, "*.txt" or "Foo???.doc".
	// However, be careful in specifying patterns that aren't cross platform
	// since the underlying code uses OS-specific matching routines.  In general,
	// Windows matching is less featureful than others, so test there first.
	// If unspecified, this will match all files.
	FileEnumerator(const FilePath& root_path,
				   bool recursive,
				   int file_type);
	FileEnumerator(const FilePath& root_path,
				   bool recursive,
				   int file_type,
				   const FilePath::StringType& pattern);
	FileEnumerator(const FilePath& root_path,
				   bool recursive,
				   int file_type,
				   const FilePath::StringType& pattern,
				   FolderSearchPolicy folder_search_policy);
	~FileEnumerator();

	// Returns the next file or an empty string if there are no more results.
	//
	// The returned path will incorporate the |root_path| passed in the
	// constructor: "<root_path>/file_name.txt". If the |root_path| is absolute,
	// then so will be the result of Next().
	FilePath next();

	// Write the file info into |info|.
	FileInfo get_info() const;

private:
	// Returns true if the given path should be skipped in enumeration.
	bool should_skip(const FilePath& path);

	bool is_type_matched(bool is_dir) const;

	bool is_pattern_matched(const FilePath& src) const;

private:
	// The files in the current directory
	std::vector<FileInfo> directory_entries_;

	// Set of visited directories. Used to prevent infinite looping along
	// circular symlinks.
	std::unordered_set<ino_t> visited_directories_;

	// The next entry to use from the directory_entries_ vector
	size_t current_directory_entry_;

	FilePath root_path_;
	const bool recursive_;
	const int file_type_;
	FilePath::StringType pattern_;
	const FolderSearchPolicy folder_search_policy_;

	// A stack that keeps track of which subdirectories we still need to
	// enumerate in the breadth-first search.
	std::stack<FilePath> pending_paths_;

	DISALLOW_COPY_AND_ASSIGN(FileEnumerator);
};

}	// namespace annety

#endif  // ANT_FILES_FILE_ENUMERATOR_H_
