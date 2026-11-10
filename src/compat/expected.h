// expected.h: C++23's std::expected for the recoverable failures (Patella-Modern issue #80, M5), under the names the
// code uses. Not part of the original source.
//
// patella::TExpected<T, E> is std::expected<T, E> and patella::TUnexpected<E> std::unexpected<E>. The code spells
// TUnexpected's argument type (no deduction guides) and tests has_value before it reads a value or an error.

#pragma once

#include <expected> // IWYU pragma: export

namespace patella {

template <class T, class E>
using TExpected = std::expected<T, E>;

template <class E>
using TUnexpected = std::unexpected<E>;

} // namespace patella
