/**
 * @file unicode.h
 * @brief Various utility classes and functions.
 */

#pragma once

#include <limits>

namespace PlainCloud::Util::Unicode {

/**
 * @brief Calculate length of Unicode code point
 *
 * @tparam Char Char type.
 * @param begin Pointer to start of unicode sequence.
 * @return Length of the code point.
 */
template<typename Char>
    requires std::is_integral_v<Char>
constexpr auto code_point_length(const Char* begin) -> int
{
    if constexpr (sizeof(Char) != 1) {
        return 1;
    } else {
        /**
         * First 5 bits of UTF-8 codepoint contain it's length, which can be from 1 to 4.
         * We can fit the length in 2 bytes by storing value from 0 to 3.
         * 5 bits contain 32 combinations, and for each we need 2 bits, this means
         * all possible values can be indexed into 64 bit integer constant.
         * We can get that integer like this:
         *
         * ```python
         * arr = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3]
         * result = 0
         * pow = 1
         * for x in arr:
         *     result += pow * x
         *     pow *= 4
         * print(f'0x{result:x}')
         * ```
         * This gives us 0x3a55000000000000.
         * That logic basically indexes into this integer, treating it as an array of 2-bit values.
         *
         * Now we can calculate the length:
         * ```cpp
         * auto length = (0x3a55000000000000 >> (2U * (chr >> 3U))) & 0x3U) + 1
         * ```
         *
         * - `(chr >> 3U)`: look at the left 5 bits ignoring the least significant.
         * - `2U *`: find corresponding index inside integer.
         * - `& 0x3U`: selects only needed two bits.
         * - `+1`: convert from 0-3 to 1-4 in order to get code point length.
         *
         * See https://emnudge.dev/blog/utf-8, https://github.com/fmtlib/fmt/pull/3333
         */

        const auto chr = static_cast<unsigned char>(*begin);
        constexpr auto CodepointLengths = 0x3a55000000000000ULL;
        return static_cast<int>((CodepointLengths >> (2U * (chr >> 3U))) & 0x3U) + 1;
    }
}

/**
 * @brief Convert character code to ASCII.
 *
 * @tparam Char Char type.
 * @param chr Character code.
 * @return \b 0 if code is greater then 0xFF, otherwise the same character code.
 */
template<typename Char>
    requires std::is_integral_v<Char>
constexpr auto to_ascii(Char chr) -> char
{
    return chr <= std::numeric_limits<unsigned char>::max() ? static_cast<char>(chr) : '\0';
}

} // namespace PlainCloud::Util::Unicode
