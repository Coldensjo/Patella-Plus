// mdspan.h: C++23's std::mdspan for the client's multi-dimensional storage (Container.h's matrix and matrix3d, the
// automap's colour and speed layers; Patella-Modern issue #101), under the names the code uses. Not part of the
// original source.
//
//	- patella::TExtents<IndexType, Extents...> is std::extents (a static extent or patella::DynamicExtent each);
//	- patella::TDynamicExtents<IndexType, Rank> is std::dextents;
//	- patella::TLayoutRight and TLayoutLeft are std::layout_right (the last index the fastest, C's arrays) and
//	  std::layout_left (the first the fastest, Fortran's);
//	- patella::TMdspan<T, Extents, Layout = TLayoutRight> is std::mdspan with the default accessor.
// Like std::mdspan in a library without hardening, it checks no precondition (an index past an extent). The
// containers check their indices before they reach a view (Container.h).
//
// A TMdspan is a view: a pointer and the dynamic extents. The code makes one from the storage that owns the elements
// (a std::unique_ptr<T[]>, a std::array) when it indexes it, and never keeps one in a structure.

#pragma once

#include <cstddef>
#include <mdspan> // IWYU pragma: export

namespace patella {

template <typename IndexType, std::size_t... Extents>
using TExtents = std::extents<IndexType, Extents...>;

template <typename IndexType, std::size_t Rank>
using TDynamicExtents = std::dextents<IndexType, Rank>;

using TLayoutRight = std::layout_right;
using TLayoutLeft = std::layout_left;

template <typename T, typename Extents, typename Layout = TLayoutRight>
using TMdspan = std::mdspan<T, Extents, Layout>;

} // namespace patella
