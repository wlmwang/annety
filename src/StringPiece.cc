// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modify: Anny Wang
// Date: May 8 2019

#include "StringPiece.h"

#include <ostream>

namespace annety {
// for testing only
std::ostream& operator<<(std::ostream& os, const StringPiece& piece) {
	return os.write(piece.data(), static_cast<std::streamsize>(piece.size()));
}

}	// namespace annety
