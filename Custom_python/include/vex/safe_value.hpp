#pragma once

#include <cmath>
#include <cstdint>
#include <type_traits>

// PROS reports an unplugged or not-ready device by returning PROS_ERR
// (INT32_MAX) or PROS_ERR_F (infinity). VEXcode returns 0 instead, so every
// device reading goes through vsafe::num() and never reaches Python as
// infinity or NaN. vsafe::div() returns 0 instead of dividing by zero.
namespace vsafe {

template <typename T>
inline std::enable_if_t<std::is_floating_point_v<T>, T> num(T v) {
    return std::isfinite(v) ? v : T(0);
}

template <typename T>
inline std::enable_if_t<std::is_integral_v<T>, T> num(T v) {
    if constexpr (sizeof(T) >= sizeof(std::int32_t) && std::is_signed_v<T>) {
        if (v == static_cast<T>(INT32_MAX)) {
            return T(0);
        }
    }
    return v;
}

inline double div(double a, double b) {
    if (b == 0.0 || !std::isfinite(a) || !std::isfinite(b)) {
        return 0.0;
    }
    const double r = a / b;
    return std::isfinite(r) ? r : 0.0;
}

inline float fnum(double v) {
    const float f = static_cast<float>(v);
    return std::isfinite(f) ? f : 0.0f;
}

}  // namespace vsafe
