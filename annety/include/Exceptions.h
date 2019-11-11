// By: wlmwang
// Date: May 28 2019

#ifndef ANT_EXCEPTIONS_H
#define ANT_EXCEPTIONS_H

#include <exception>
#include <string>

namespace annety
{
std::string backtrace_to_string(bool demangle);

class Exceptions : public std::exception
{
public:
	Exceptions(std::string what);
	~Exceptions() noexcept override = default;

	const char* what() const noexcept override
	{
		return message_.c_str();
	}

	const char* backtrace() const noexcept
	{
		return stack_.c_str();
	}

private:
	std::string message_;
	std::string stack_;
};

}  // namespace annety

#endif  // ANT_EXCEPTIONS_H
