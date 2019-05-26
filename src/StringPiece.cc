

#include <ostream>

#include "StringPiece.h"

namespace annety {
// For testing only
std::ostream& operator<<(std::ostream& os, const StringPiece& piece) {
	return os.write(piece.data(), static_cast<std::streamsize>(piece.size()));
}

}	// namespace annety


