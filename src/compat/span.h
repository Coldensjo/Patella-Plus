// span.h: C++20's std::span for the parameters that took a pointer and a length (the packet buffers, TXTEA and TRSA;
// Patella-Modern issue #59), under the names the code uses. Not part of the original source.
//
// patella::TSpan<T, Extent> is std::span<T, Extent>, and patella::DynamicExtent is std::dynamic_extent.
//
// The size is std::span's in MSVC's, libstdc++'s and libc++'s library: a pointer for a fixed extent, a pointer and a
// std::size_t for the dynamic one, with a pointer's alignment, trivially copyable (asserted below). A TSpan is a
// parameter or a local only, never a member of a frozen structure (plan, section 1.2).

#pragma once

#include <cstddef>
#include <span> // IWYU pragma: export
#include <type_traits>

namespace patella {

inline constexpr std::size_t DynamicExtent = std::dynamic_extent;

template <typename T, std::size_t Extent = std::dynamic_extent>
using TSpan = std::span<T, Extent>;

// The layout (a parameter's, never a frozen member's).
template <typename T, std::size_t Extent>
inline constexpr bool IsSpanLayout = sizeof(TSpan<T, Extent>)
		== (Extent == DynamicExtent ? sizeof(T *) + sizeof(std::size_t) : sizeof(T *))
	&& alignof(TSpan<T, Extent>) == alignof(T *) && std::is_trivially_copyable_v<TSpan<T, Extent>>;
static_assert(IsSpanLayout<unsigned char, DynamicExtent> && IsSpanLayout<const unsigned char, DynamicExtent>
	&& IsSpanLayout<char, DynamicExtent> && IsSpanLayout<unsigned char, 8> && IsSpanLayout<const unsigned char, 16>
	&& IsSpanLayout<unsigned char, 128>);

} // namespace patella
