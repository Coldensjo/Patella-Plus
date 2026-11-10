// AutomapLayer.h: one layer of an automap block, 256 x 256 bytes of one kind (Map.h's TAutomapBlock: the colours and
// the speeds), owned by the layer and indexed through a std::mdspan view.
//
// The bytes are in the order of the automap file (<bx><by><bz>.map), which LoadAutomapBlock and SaveAutomapBlock read
// and write in one piece: x the slowest index, y the fastest, the byte of x, y at x * 256 + y. The view is layout_right
// with the extents 256, 256, so view()[x, y] is that byte; Map.cpp checks it against the file's layout.

#pragma once

#include "common.h"
#include "compat/mdspan.h"

#include <array>
#include <cstddef>

inline constexpr int AUTOMAP_BLOCK_SIZE = 256; // fields of a block along x and along y

struct TAutomapLayer {
	using TView = patella::TMdspan<uint8, patella::TExtents<int, AUTOMAP_BLOCK_SIZE, AUTOMAP_BLOCK_SIZE>>;
	using TConstView = patella::TMdspan<const uint8, patella::TExtents<int, AUTOMAP_BLOCK_SIZE, AUTOMAP_BLOCK_SIZE>>;

	// [x, y], 0 to 255 each.
	TView view() { return TView(Bytes.data()); }
	[[nodiscard]] TConstView view() const { return TConstView(Bytes.data()); }

	std::array<uint8, static_cast<std::size_t>(AUTOMAP_BLOCK_SIZE) * AUTOMAP_BLOCK_SIZE> Bytes;
};
