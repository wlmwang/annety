// Refactoring: Anny Wang
// Date: May 28 2019

#ifndef ANT_EXCEPTION_H
#define ANT_EXCEPTION_H

#include "CompilerSpecific.h"

#include <exception>
#include <string>

namespace annety {
std::string backtrace_to_string(bool demangle);

class Exception : public std::exception {
public:
  Exception(std::string what);
  ~Exception() noexcept override = default;

  const char* what() const noexcept override {
    return message_.c_str();
  }
  
  const char* backtrace() const noexcept {
    return stack_.c_str();
  }

private:
  std::string message_;
  std::string stack_;
};

}  // namespace annety

#endif  // ANT_EXCEPTION_H
