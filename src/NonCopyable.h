// Modify: Anny Wang
// Date: May 8 2019

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

