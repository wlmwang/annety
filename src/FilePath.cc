// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Refactoring: Anny Wang
// Date: Jun 04 2019

#include "FilePath.h"
#include "Macros.h"
#include "Logging.h"
#include "StringPiece.h"
#include "StringUtil.h"

#include <string.h>		// strcasecmp
#include <string>
#include <vector>

namespace annety {
using StringType = FilePath::StringType;
using StringPieceType = FilePath::StringPieceType;

const char FilePath::kSeparators[] = "/";
const char FilePath::kCurrentDirectory[] = ".";
const char FilePath::kParentDirectory[] = "..";
const char FilePath::kExtensionSeparator = '.';
const size_t FilePath::kSeparatorsLength = arraysize(kSeparators);

namespace {
const char* const kCommonDoubleExtensionSuffixes[] = { "gz", "z", "bz2", "bz" };
const char* const kCommonDoubleExtensions[] = { "user.js" };

const char kStringTerminator = '\0';

// If this FilePath contains a drive letter specification, returns the
// position of the last character of the drive letter specification,
// otherwise returns npos.  This can only be true on Windows, when a pathname
// begins with a letter followed by a colon.  On other platforms, this always
// returns npos.
StringPieceType::size_type find_drive_letter(StringPieceType path) {
	return StringType::npos;
}

bool is_path_absolute(StringPieceType path) {
	// Look for a separator in the first position.
	return path.length() > 0 && FilePath::is_separator(path[0]);
}

bool are_all_separators(const StringType& input) {
	for (StringType::const_iterator it = input.begin();
		it != input.end(); ++it)
	{
		if (!FilePath::is_separator(*it)) {
			return false;
		}
	}
	return true;
}

// Find the position of the '.' that separates the extension from the rest
// of the file name. The position is relative to BaseName(), not value().
// Returns npos if it can't find an extension.
StringType::size_type final_extension_separator_position(const StringType& path) {
	// Special case "." and ".."
	if (path == FilePath::kCurrentDirectory || 
		path == FilePath::kParentDirectory)
	{
		return StringType::npos;
	}

	return path.rfind(FilePath::kExtensionSeparator);
}

// Same as above, but allow a second extension component of up to 4
// characters when the rightmost extension component is a common double
// extension (gz, bz2, Z).  For example, foo.tar.gz or foo.tar.Z would have
// extension components of '.tar.gz' and '.tar.Z' respectively.
StringType::size_type extension_separator_position(const StringType& path) {
	const StringType::size_type last_dot = final_extension_separator_position(path);

	// No extension, or the extension is the whole filename.
	if (last_dot == StringType::npos || last_dot == 0U) {
		return last_dot;
	}

	const StringType::size_type penultimate_dot =
			path.rfind(FilePath::kExtensionSeparator, last_dot - 1);
	const StringType::size_type last_separator =
			path.find_last_of(FilePath::kSeparators, last_dot - 1,
							  FilePath::kSeparatorsLength - 1);

	if (penultimate_dot == StringType::npos ||
		(last_separator != StringType::npos &&
		penultimate_dot < last_separator))
	{
		return last_dot;
	}

	for (size_t i = 0; i < arraysize(kCommonDoubleExtensions); ++i) {
		StringType ext(path, penultimate_dot + 1);
		if (lower_case_equals(ext, kCommonDoubleExtensions[i])) {
			return penultimate_dot;
		}
	}

	StringType ext(path, last_dot + 1);
	for (size_t i = 0; i < arraysize(kCommonDoubleExtensionSuffixes); ++i) {
		if (lower_case_equals(ext, kCommonDoubleExtensionSuffixes[i])) {
			if ((last_dot - penultimate_dot) <= 5U &&
				(last_dot - penultimate_dot) > 1U)
			{
				return penultimate_dot;
			}
		}
	}

	return last_dot;
}

// Returns true if path is "", ".", or "..".
bool is_empty_or_special_case(const StringType& path) {
	// Special cases "", ".", and ".."
	if (path.empty() || path == FilePath::kCurrentDirectory ||
		path == FilePath::kParentDirectory)
	{
		return true;
	}

	return false;
}

}	// namespace anonymous


FilePath::FilePath() = default;

FilePath::FilePath(const FilePath& that) = default;
FilePath::FilePath(FilePath&& that) noexcept = default;

FilePath::FilePath(StringPieceType path) {
	path.copy_to_string(&path_);
	StringType::size_type nul_pos = path_.find(kStringTerminator);
	if (nul_pos != StringType::npos) {
		path_.erase(nul_pos, StringType::npos);
	}
}

FilePath::~FilePath() = default;

FilePath& FilePath::operator=(const FilePath& that) = default;

FilePath& FilePath::operator=(FilePath&& that) = default;

bool FilePath::operator==(const FilePath& that) const {
  return path_ == that.path_;
}

bool FilePath::operator!=(const FilePath& that) const {
  return path_ != that.path_;
}

std::ostream& operator<<(std::ostream& out, const FilePath& file_path) {
	return out << file_path.value();
}

// static
bool FilePath::is_separator(char character) {
	for (size_t i = 0; i < kSeparatorsLength - 1; ++i) {
		if (character == kSeparators[i]) {
			return true;
		}
	}

	return false;
}

void FilePath::get_components(std::vector<StringType>* components) const {
	DCHECK(components);

	if (!components) {
		return;
	}
	components->clear();

	if (value().empty()) {
		return;
	}

	std::vector<StringType> ret_val;
	FilePath current = *this;
	FilePath base;

	// Capture path components.
	while (current != current.dirname()) {
		base = current.basename();
		if (!are_all_separators(base.value())) {
			ret_val.push_back(base.value());
		}
		current = current.dirname();
	}

	// Capture root, if any.
	base = current.basename();
	if (!base.value().empty() && base.value() != kCurrentDirectory) {
		ret_val.push_back(current.basename().value());
	}

	// Capture drive letter, if any.
	FilePath dir = current.dirname();
	StringType::size_type letter = find_drive_letter(dir.value());
	if (letter != StringType::npos) {
		ret_val.push_back(StringType(dir.value(), 0, letter + 1));
	}

	*components = std::vector<StringType>(ret_val.rbegin(), ret_val.rend());
}

bool FilePath::is_parent(const FilePath& child) const {
	return append_relative_path(child, nullptr);
}

bool FilePath::append_relative_path(const FilePath& child,
									FilePath* path) const {
	std::vector<StringType> parent_components;
	std::vector<StringType> child_components;
	get_components(&parent_components);
	child.get_components(&child_components);

	if (parent_components.empty() || 
		parent_components.size() >= child_components.size())
	{
		return false;
	}

	std::vector<StringType>::const_iterator parent_comp =
		parent_components.begin();
	std::vector<StringType>::const_iterator child_comp =
		child_components.begin();

	while (parent_comp != parent_components.end()) {
		if (*parent_comp != *child_comp) {
			return false;
		}
		++parent_comp;
		++child_comp;
	}

	if (path != nullptr) {
		for (; child_comp != child_components.end(); ++child_comp) {
			*path = path->append(*child_comp);
		}
	}

	return true;
}

// libgen's dirname and basename aren't guaranteed to be thread-safe and aren't
// guaranteed to not modify their input strings, and in fact are implemented
// differently in this regard on different platforms.  Don't use them, but
// adhere to their behavior.
FilePath FilePath::dirname() const {
	FilePath new_path(path_);
	new_path.strip_trailing_separators_internal();

	// The drive letter, if any, always needs to remain in the output.  If there
	// is no drive letter, as will always be the case on platforms which do not
	// support drive letters, letter will be npos, or -1, so the comparisons and
	// resizes below using letter will still be valid.
	StringType::size_type letter = find_drive_letter(new_path.path_);

	StringType::size_type last_separator =
		new_path.path_.find_last_of(kSeparators, StringType::npos,
									kSeparatorsLength - 1);
	if (last_separator == StringType::npos) {
		// path_ is in the current directory.
		new_path.path_.resize(letter + 1);
	} else if (last_separator == letter + 1) {
		// path_ is in the root directory.
		new_path.path_.resize(letter + 2);
	} else if (last_separator == letter + 2 &&
			   is_separator(new_path.path_[letter + 1]))
	{
		// path_ is in "//" (possibly with a drive letter); leave the double
		// separator intact indicating alternate root.
		new_path.path_.resize(letter + 3);
	} else if (last_separator != 0) {
		// path_ is somewhere else, trim the basename.
		new_path.path_.resize(last_separator);
	}

	new_path.strip_trailing_separators_internal();
	if (!new_path.path_.length()) {
		new_path.path_ = kCurrentDirectory;
	}

	return new_path;
}

FilePath FilePath::basename() const {
	FilePath new_path(path_);
	new_path.strip_trailing_separators_internal();

	// The drive letter, if any, is always stripped.
	StringType::size_type letter = find_drive_letter(new_path.path_);
	if (letter != StringType::npos) {
		new_path.path_.erase(0, letter + 1);
	}

	// Keep everything after the final separator, but if the pathname is only
	// one character and it's a separator, leave it alone.
	StringType::size_type last_separator =
		new_path.path_.find_last_of(kSeparators, StringType::npos,
									kSeparatorsLength - 1);
	if (last_separator != StringType::npos &&
		last_separator < new_path.path_.length() - 1) {
		new_path.path_.erase(0, last_separator + 1);
	}

	return new_path;
}

StringType FilePath::extension() const {
	FilePath base(basename());

	const StringType::size_type dot = extension_separator_position(base.path_);
	if (dot == StringType::npos) {
		return StringType();
	}

	return base.path_.substr(dot, StringType::npos);
}

StringType FilePath::final_extension() const {
	FilePath base(basename());

	const StringType::size_type dot = final_extension_separator_position(base.path_);
	if (dot == StringType::npos) {
		return StringType();
	}

	return base.path_.substr(dot, StringType::npos);
}

FilePath FilePath::remove_extension() const {
	if (extension().empty()) {
		return *this;
	}

	const StringType::size_type dot = extension_separator_position(path_);
	if (dot == StringType::npos) {
		return *this;
	}

	return FilePath(path_.substr(0, dot));
}

FilePath FilePath::remove_final_extension() const {
	if (final_extension().empty()) {
		return *this;
	}

	const StringType::size_type dot = final_extension_separator_position(path_);
	if (dot == StringType::npos) {
		return *this;
	}

	return FilePath(path_.substr(0, dot));
}

FilePath FilePath::insert_before_extension(StringPieceType suffix) const {
	if (suffix.empty()) {
		return FilePath(path_);
	}

	if (is_empty_or_special_case(basename().value())) {
		return FilePath();
	}

	StringType ext = extension();
	StringType ret = remove_extension().value();
	suffix.append_to_string(&ret);
	ret.append(ext);
	return FilePath(ret);
}

FilePath FilePath::add_extension(StringPieceType ext) const {
	if (is_empty_or_special_case(basename().value())) {
		return FilePath();
	}

	// If the new ext is "" or ".", then just return the current FilePath.
	if (ext.empty() || 
		(ext.size() == 1 && ext[0] == kExtensionSeparator))
	{
		return *this;
	}

	StringType str = path_;
	if (ext[0] != kExtensionSeparator &&
		*(str.end() - 1) != kExtensionSeparator)
	{
		str.append(1, kExtensionSeparator);
	}
	ext.append_to_string(&str);
	return FilePath(str);
}

FilePath FilePath::replace_extension(StringPieceType ext) const {
	if (is_empty_or_special_case(basename().value())) {
		return FilePath();
	}

	FilePath no_ext = remove_extension();
	// If the new ext is "" or ".", then just remove the current ext.
	if (ext.empty() || 
		(ext.size() == 1 && ext[0] == kExtensionSeparator)) {
		return no_ext;
	}

	StringType str = no_ext.value();
	if (ext[0] != kExtensionSeparator) {
		str.append(1, kExtensionSeparator);
	}
	ext.append_to_string(&str);
	return FilePath(str);
}

bool FilePath::matches_extension(StringPieceType ext) const {
	DCHECK(ext.empty() || ext[0] == kExtensionSeparator);

	StringType current_extension = extension();

	if (current_extension.length() != ext.length()) {
		return false;
	}

	return FilePath::compare_equal_ignore_case(ext, current_extension);
}

FilePath FilePath::append(StringPieceType component) const {
	StringPieceType appended = component;
	StringType without_nuls;

	StringType::size_type nul_pos = component.find(kStringTerminator);
	if (nul_pos != StringPieceType::npos) {
		component.substr(0, nul_pos).copy_to_string(&without_nuls);
		appended = StringPieceType(without_nuls);
	}

	DCHECK(!is_path_absolute(appended));

	if (path_.compare(kCurrentDirectory) == 0 && !appended.empty()) {
		// Append normally doesn't do any normalization, but as a special case,
		// when appending to kCurrentDirectory, just return a new path for the
		// component argument.  Appending component to kCurrentDirectory would
		// serve no purpose other than needlessly lengthening the path, and
		// it's likely in practice to wind up with FilePath objects containing
		// only kCurrentDirectory when calling DirName on a single relative path
		// component.
		return FilePath(appended);
	}

	FilePath new_path(path_);
	new_path.strip_trailing_separators_internal();

	// Don't append a separator if the path is empty (indicating the current
	// directory) or if the path component is empty (indicating nothing to
	// append).
	if (!appended.empty() && !new_path.path_.empty()) {
		// Don't append a separator if the path still ends with a trailing
		// separator after stripping (indicating the root directory).
		if (!is_separator(new_path.path_.back())) {
			// Don't append a separator if the path is just a drive letter.
			if (find_drive_letter(new_path.path_) + 1 != new_path.path_.length()) {
				new_path.path_.append(1, kSeparators[0]);
			}
		}
	}

	appended.append_to_string(&new_path.path_);
	return new_path;
}

FilePath FilePath::append(const FilePath& component) const {
	return append(component.value());
}

bool FilePath::is_absolute() const {
	return is_path_absolute(path_);
}

bool FilePath::ends_with_separator() const {
	if (empty()) {
		return false;
	}
	return is_separator(path_.back());
}

FilePath FilePath::as_ending_with_separator() const {
	if (ends_with_separator() || path_.empty()) {
		return *this;
	}

	StringType path_str;
	path_str.reserve(path_.length() + 1);  // Only allocate string once.

	path_str = path_;
	path_str.append(&kSeparators[0], 1);
	return FilePath(path_str);
}

FilePath FilePath::strip_trailing_separators() const {
	FilePath new_path(path_);
	new_path.strip_trailing_separators_internal();

	return new_path;
}

bool FilePath::references_parent() const {
	if (path_.find(kParentDirectory) == StringType::npos) {
		// GetComponents is quite expensive, so avoid calling it in the majority
		// of cases where there isn't a kParentDirectory anywhere in the path.
		return false;
	}

	std::vector<StringType> components;
	get_components(&components);

	std::vector<StringType>::const_iterator it = components.begin();
	for (; it != components.end(); ++it) {
		const StringType& component = *it;
		// Windows has odd, undocumented behavior with path components containing
		// only whitespace and . characters. So, if all we see is . and
		// whitespace, then we treat any .. sequence as referencing parent.
		// For simplicity we enforce this on all platforms.
		if (component.find_first_not_of(". \n\r\t") == std::string::npos &&
			component.find(kParentDirectory) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

#if defined(OS_MACOSX)
int FilePath::compare_ignore_case(StringPieceType string1,
								  StringPieceType string2) {
	// Specifically need null termianted strings for this API call.
	int comparison = ::strcasecmp(string1.as_string().c_str(),
								  string2.as_string().c_str());
	if (comparison < 0) {
		return -1;
	}
	if (comparison > 0) {
		return 1;
	}
	return 0;
}

#elif defined(OS_POSIX)

// Generic Posix system comparisons.
int FilePath::compare_ignore_case(StringPieceType string1,
								  StringPieceType string2) {
	// Specifically need null termianted strings for this API call.
	int comparison = ::strcasecmp(string1.as_string().c_str(),
					 			  string2.as_string().c_str());
	if (comparison < 0) {
		return -1;
	}
	if (comparison > 0) {
		return 1;
	}
	return 0;
}

#endif  // OS versions of CompareIgnoreCase()


void FilePath::strip_trailing_separators_internal() {
	// If there is no drive letter, start will be 1, which will prevent stripping
	// the leading separator if there is only one separator.  If there is a drive
	// letter, start will be set appropriately to prevent stripping the first
	// separator following the drive letter, if a separator immediately follows
	// the drive letter.
	StringType::size_type start = find_drive_letter(path_) + 2;

	StringType::size_type last_stripped = StringType::npos;
	for (StringType::size_type pos = path_.length();
		 pos > start && is_separator(path_[pos - 1]); --pos)
	{
		// If the string only has two separators and they're at the beginning,
		// don't strip them, unless the string began with more than two separators.
		if (pos != start + 1 || last_stripped == start + 2 ||
			!is_separator(path_[start - 1]))
		{
			path_.resize(pos - 1);
			last_stripped = pos;
		}
	}
}

FilePath FilePath::normalize_path_separators() const {
	return normalize_path_separators_to(kSeparators[0]);
}

FilePath FilePath::normalize_path_separators_to(char separator) const {
	return *this;
}


}	// namespace annety
