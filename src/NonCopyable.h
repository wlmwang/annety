// Copyright (c) 2018 Anny Wang Authors. All rights reserved.

#ifndef ANT_NONCOPYABLE_H
#define ANT_NONCOPYABLE_H

namespace annety {

class NonCopyable {
public:
	NonCopyable(const NonCopyable&) = delete;
	void operator=(const NonCopyable&) = delete;

protected:
	NonCopyable() = default;
	~NonCopyable() = default;
};

}	// namespace annety

#endif  // ANT_NONCOPYABLE_H

