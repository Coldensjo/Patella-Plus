// TXTEA: the XTEA cipher used for the session. The original file name is
// unknown; "TXTEA" fits the alphabetical link order (after TRSA, before Utils).

#pragma once

#include "common.h"
#include "compat/span.h"
#include <array>

class TXTEA {
public:
	// The key's 16 bytes and a block's 8 (the original took pointers). A span whose pointer is null is ignored, as the
	// original's null pointer was.
	using TKey = patella::TSpan<const uint8, 16>;
	using TBlock = patella::TSpan<uint8, 8>;

	TXTEA();
	void SetKey(TKey Key);
	void EncryptBlock(TBlock Block); // in place
	void DecryptBlock(TBlock Block);

private:
	std::array<uint32, 4> Key;
};
