

#ifndef ANT_SAFE_STRERROR_H_
#define ANT_SAFE_STRERROR_H_

#include <stddef.h>
#include <string>

#include "StringPiece.h"

namespace annety {

// Use this instead of strerror_r().
StringPiece safe_strerror_r(int err, char *buf, size_t len);

// Calls safe_strerror_r with a buffer of suitable size and returns the result
// in a C++ string.
//
// Use this instead of strerror(). Note though that safe_strerror_r will be
// more robust in the case of heap corruption errors, since it doesn't need to
// allocate a string.
std::string safe_strerror(int err);

}  // namespace annety

#endif  // ANT_SAFE_STRERROR_H_

