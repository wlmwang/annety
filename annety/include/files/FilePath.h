// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// By: wlmwang
// Date: Jun 04 2019

#ifndef ANT_FILES_FILE_PATH_H_
#define ANT_FILES_FILE_PATH_H_

#include "build/CompilerSpecific.h"	// WARN_UNUSED_RESULT
#include "strings/StringPiece.h"

#include <iosfwd>
#include <string>
#include <vector>
#include <functional>	// std::hash
#include <stddef.h>		// size_t

namespace annety
{
// Example:
// // FilePath
// FilePath fp("/usr/local/annety.log.gz");
// cout << "full:" << fp << endl;
// cout << "dirname:" << fp.dirname() << endl;
// cout << "basename:" << fp.basename() << endl;
// cout << "extension:" << fp.extension() << endl;
// cout << "final extension:" << fp.final_extension() << endl;
//
// cout << "scan all components:" << endl;
// std::vector<std::string> components;
// fp.get_components(&components);
// for (auto cc : components) {
//		cout << "|" << cc << "|";
// }
// cout << endl;
// ...


// An abstraction to isolate users from the differences between native
// pathnames on different platforms.
class FilePath
{
public:
	typedef std::string StringType;
	typedef StringPiece StringPieceType;
	
	// Null-terminated array of separators used to separate components in
	// hierarchical paths.  Each character in this array is a valid separator,
	// but kSeparators[0] is treated as the canonical separator and will be used
	// when composing pathnames.
	static const char kSeparators[];
	static const size_t kSeparatorsLength;

	// A special path component meaning "this directory."
	static const char kCurrentDirectory[];

	// A special path component meaning "the parent directory."
	static const char kParentDirectory[];

	// The character used to identify a file extension.
	static const char kExtensionSeparator;

public:
	FilePath();
	explicit FilePath(StringPiece path);
	
	~FilePath();

	FilePath(const FilePath& that);
	FilePath& operator=(const FilePath& that);

	// Constructs FilePath with the contents of |that|, which is left in valid but
	// unspecified state.
	FilePath(FilePath&& that) noexcept;
	
	// Replaces the contents with those of |that|, which is left in valid but
	// unspecified state.
	FilePath& operator=(FilePath&& that);

	bool operator==(const FilePath& that) const;
	bool operator!=(const FilePath& that) const;

	// Required for some STL containers and operations
	bool operator<(const FilePath& that) const
	{
		return path_ < that.path_;
	}

	const std::string& value() const { return path_;}

	bool empty() const { return path_.empty();}

	void clear() { path_.clear();}

	// Returns true if |character| is in kSeparators.
	static bool is_separator(char character);

	// Returns a vector of all of the components of the provided path. It is
	// equivalent to calling DirName().value() on the path's root component,
	// and BaseName().value() on each child component.
	//
	// To make sure this is lossless so we can differentiate absolute and
	// relative paths, the root slash will be included even though no other
	// slashes will be. The precise behavior is:
	//
	// Posix:  "/foo/bar"  ->  [ "/", "foo", "bar" ]
	void get_components(std::vector<std::string>* components) const;

	// Returns true if this FilePath is a strict parent of the |child|. Absolute
	// and relative paths are accepted i.e. is /foo parent to /foo/bar and
	// is foo parent to foo/bar. Does not convert paths to absolute, follow
	// symlinks or directory navigation (e.g. ".."). A path is *NOT* its own
	// parent.
	bool is_parent(const FilePath& child) const;

	// If IsParent(child) holds, appends to path (if non-NULL) the
	// relative path to child and returns true.  For example, if parent
	// holds "/Users/johndoe/Library/Application Support", child holds
	// "/Users/johndoe/Library/Application Support/Google/Chrome/Default", and
	// *path holds "/Users/johndoe/Library/Caches", then after
	// parent.AppendRelativePath(child, path) is called *path will hold
	// "/Users/johndoe/Library/Caches/Google/Chrome/Default".  Otherwise,
	// returns false.
	bool append_relative_path(const FilePath& child, FilePath* path) const;

	// Returns a FilePath corresponding to the directory containing the path
	// named by this object, stripping away the file component.  If this object
	// only contains one component, returns a FilePath identifying
	// kCurrentDirectory.  If this object already refers to the root directory,
	// returns a FilePath identifying the root directory. Please note that this
	// doesn't resolve directory navigation, e.g. the result for "../a" is "..".
	FilePath dirname() const WARN_UNUSED_RESULT;

	// Returns a FilePath corresponding to the last path component of this
	// object, either a file or a directory.  If this object already refers to
	// the root directory, returns a FilePath identifying the root directory;
	// this is the only situation in which BaseName will return an absolute path.
	FilePath basename() const WARN_UNUSED_RESULT;

	// Returns ".jpg" for path "C:\pics\jojo.jpg", or an empty string if
	// the file has no extension.  If non-empty, extension() will always start
	// with precisely one ".".  The following code should always work regardless
	// of the value of path.  For common double-extensions like .tar.gz and
	// .user.js, this method returns the combined extension.  For a single
	// component, use final_extension().
	// new_path = path.remove_extension().value().append(path.extension());
	// ASSERT(new_path == path.value());
	// NOTE: this is different from the original file_util implementation which
	// returned the extension without a leading "." ("jpg" instead of ".jpg")
	std::string extension() const WARN_UNUSED_RESULT;

	// Returns the path's file extension, as in extension(), but will
	// never return a double extension.
	//
	// TODO(davidben): Check all our extension-sensitive code to see if
	// we can rename this to extension() and the other to something like
	// LongExtension(), defaulting to short extensions and leaving the
	// long "extensions" to logic like base::GetUniquePathNumber().
	std::string final_extension() const WARN_UNUSED_RESULT;

	// Returns "C:\pics\jojo" for path "C:\pics\jojo.jpg"
	// NOTE: this is slightly different from the similar file_util implementation
	// which returned simply 'jojo'.
	FilePath remove_extension() const WARN_UNUSED_RESULT;

	// Removes the path's file ext, as in RemoveExtension(), but
	// ignores double extensions.
	FilePath remove_final_extension() const WARN_UNUSED_RESULT;

	// Inserts |suffix| after the file name portion of |path| but before the
	// ext.  Returns "" if BaseName() == "." or "..".
	// Examples:
	// path == "C:\pics\jojo.jpg" suffix == " (1)", returns "C:\pics\jojo (1).jpg"
	// path == "jojo.jpg"         suffix == " (1)", returns "jojo (1).jpg"
	// path == "C:\pics\jojo"     suffix == " (1)", returns "C:\pics\jojo (1)"
	// path == "C:\pics.old\jojo" suffix == " (1)", returns "C:\pics.old\jojo (1)"
	FilePath insert_before_extension(StringPiece suffix) const WARN_UNUSED_RESULT;

	// Adds |ext| to |file_name|. Returns the current FilePath if
	// |ext| is empty. Returns "" if BaseName() == "." or "..".
	FilePath add_extension(StringPiece ext) const WARN_UNUSED_RESULT;

	// Replaces the ext of |file_name| with |ext|.  If |file_name|
	// does not have an ext, then |ext| is added.  If |ext| is
	// empty, then the ext is removed from |file_name|.
	// Returns "" if BaseName() == "." or "..".
	FilePath replace_extension(StringPiece ext) const WARN_UNUSED_RESULT;

	// Returns true if the file path matches the specified ext. The test is
	// case insensitive. Don't forget the leading period if appropriate.
	bool matches_extension(StringPiece ext) const;

	// Returns a FilePath by appending a separator and the supplied path
	// component to this object's path.  Append takes care to avoid adding
	// excessive separators if this object's path already ends with a separator.
	// If this object's path is kCurrentDirectory, a new FilePath corresponding
	// only to |component| is returned.  |component| must be a relative path;
	// it is an error to pass an absolute path.
	FilePath append(StringPiece component) const WARN_UNUSED_RESULT;
	FilePath append(const FilePath& component) const WARN_UNUSED_RESULT;

	// Returns true if this FilePath contains an absolute path.  On Windows, an
	// absolute path begins with either a drive letter specification followed by
	// a separator character, or with two separator characters.  On POSIX
	// platforms, an absolute path begins with a separator character.
	bool is_absolute() const;

	// Returns true if the patch ends with a path separator character.
	bool ends_with_separator() const WARN_UNUSED_RESULT;

	// Returns a copy of this FilePath that ends with a trailing separator. If
	// the input path is empty, an empty FilePath will be returned.
	FilePath as_ending_with_separator() const WARN_UNUSED_RESULT;

	// Returns a copy of this FilePath that does not end with a trailing
	// separator.
	FilePath strip_trailing_separators() const WARN_UNUSED_RESULT;

	// Returns true if this FilePath contains an attempt to reference a parent
	// directory (e.g. has a path component that is "..").
	bool references_parent() const;

	// Normalize all path separators to backslash on Windows
	// (if FILE_PATH_USES_WIN_SEPARATORS is true), or do nothing on POSIX systems.
	FilePath normalize_path_separators() const;

	// Normalize all path separattors to given type on Windows
	// (if FILE_PATH_USES_WIN_SEPARATORS is true), or do nothing on POSIX systems.
	FilePath normalize_path_separators_to(char separator) const;

	// Compare two strings in the same way the file system does.
	// Note that these always ignore case, even on file systems that are case-
	// sensitive. If case-sensitive comparison is ever needed, add corresponding
	// methods here.
	// The methods are written as a static method so that they can also be used
	// on parts of a file path, e.g., just the extension.
	// compare_ignore_case() returns -1, 0 or 1 for less-than, equal-to and
	// greater-than respectively.
	static int compare_ignore_case(StringPiece string1,
								   StringPiece string2);

	static bool compare_equal_ignore_case(StringPiece string1,
										  StringPiece string2)
	{
		return compare_ignore_case(string1, string2) == 0;
	}
	static bool compare_less_ignore_case(StringPiece string1,
										 StringPiece string2)
	{
		return compare_ignore_case(string1, string2) < 0;
	}

private:
	// Remove trailing separators from this object.  If the path is absolute, it
	// will never be stripped any more than to refer to the absolute root
	// directory, so "////" will become "/", not "".  A leading pair of
	// separators is never stripped, to support alternate roots.  This is used to
	// support UNC paths on Windows.
	void strip_trailing_separators_internal();

	std::string path_;
};

std::ostream& operator<<(std::ostream& out, const FilePath& file_path);

}	// namespace annety

// Hashing ---------------------------------------------------------------------

// We provide appropriate hash functions, so FilePath can be used as keys in 
// hash sets and maps.
//
// FIXME: Modify std namespace does not strictly conform to specifications.
// There is a better way (not simpler way), see: strings/StringPiece.h
namespace std {
template <>
struct hash<annety::FilePath>
{
	typedef annety::FilePath argument_type;
	typedef size_t result_type;
	
	result_type operator()(argument_type const& f) const
	{
		return std::hash<std::string>()(f.value());
	}
};

}	// namespace std

#endif	// ANT_FILES_FILE_PATH_H_
