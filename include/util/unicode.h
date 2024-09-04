/**
 * @file util.h
 * @brief Various utility classes and functions.
 */

#pragma once

#include <type_traits>

namespace PlainCloud::Util {

namespace Unicode {
template<typename Char>
constexpr inline auto code_point_length(const Char* begin) -> int
{
    if constexpr (sizeof(Char) != 1)
        return 1;
    auto c = static_cast<unsigned char>(*begin);
    return static_cast<int>((0x3a55000000000000ull >> (2 * (c >> 3))) & 0x3) + 1;
}

template<typename Char>
    requires std::is_integral<Char>::value
constexpr inline auto to_ascii(Char c) -> char
{
    return c <= 0xff ? static_cast<char>(c) : '\0';
}

} // namespace Unicode

} // namespace PlainCloud::Util