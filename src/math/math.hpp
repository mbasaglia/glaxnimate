#pragma once

namespace math {

template<class T>
constexpr T sign(T v) noexcept { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

} // namespace math
