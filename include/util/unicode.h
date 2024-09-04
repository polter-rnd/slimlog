/**
 * @file util.h
 * @brief Various utility classes and functions.
 */

#pragma once

#include <type_traits>

namespace PlainCloud::Util::Unicode {

template<typename Char>
constexpr inline auto code_point_length(const Char* begin) -> int
{
    if constexpr (sizeof(Char) != 1) {
        return 1;
    }
    auto chr = static_cast<unsigned char>(*begin);
    return static_cast<int>((0x3a55000000000000ULL >> (2U * (chr >> 3U))) & 0x3U) + 1;
}

template<typename Char>
    requires std::is_integral_v<Char>
constexpr inline auto to_ascii(Char chr) -> char
{
    return chr <= 0xff ? static_cast<char>(chr) : '\0';
}

} // namespace PlainCloud::Util::Unicode
