/**
 * @file unicode.h
 * @brief Provides various utility classes and functions for Unicode handling.
 */

#pragma once

#include <cstddef>
#include <iterator>
#include <limits>

namespace PlainCloud::Util::Unicode {

/**
 * @brief Calculates the length of a Unicode code point.
 *
 * @tparam Char Character type.
 * @param begin Pointer to the start of the Unicode sequence.
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
         * Determines the length of a UTF-8 code point based on its first 5 bits.
         * We can fit the length in 2 bytes by storing value from 0 to 3.
         * 5 bits contain 32 combinations, and for each we need 2 bits, this means
         * all possible values can be indexed into 64 bit integer constant.
         * We can get that integer like this:
         *
         * Example calculation:
         * ```cpp
         * auto length = (0x3a55000000000000 >> (2U * (chr >> 3U))) & 0x3U) + 1;
         * ```
         *
         * - `(chr >> 3U)`: Considers the leftmost 5 bits, ignoring the least significant bits.
         * - `2U *`: Computes the corresponding index within the 64-bit integer.
         * - `& 0x3U`: Extracts the relevant 2-bit value.
         * - `+ 1`: Converts the value from a range of 0-3 to 1-4, representing the code point
         * length.
         *
         * See https://emnudge.dev/blog/utf-8, https://github.com/fmtlib/fmt/pull/3333
         */

        const auto chr = static_cast<unsigned char>(*begin);
        constexpr auto CodepointLengths = 0x3a55000000000000ULL;
        return static_cast<int>((CodepointLengths >> (2U * (chr >> 3U))) & 0x3U) + 1;
    }
}

/**
 * @brief Calculates number of Unicode code points in a source data.
 *
 * @tparam Char Character type.
 * @param begin Pointer to the start of the Unicode sequence.
 * @param len Number of bytes in a source data.
 */
template<typename Char>
constexpr auto count_codepoints(const Char* begin, std::size_t len) -> std::size_t
{
    if constexpr (sizeof(Char) != 1) {
        return len;
    } else {
        std::size_t codepoints = 0;
        for (const auto* end = std::next(begin, len); begin != end; ++codepoints) {
            std::advance(begin, Util::Unicode::code_point_length(begin));
        }
        return codepoints;
    }
}

/**
 * @brief Converts a character code to ASCII.
 *
 * @tparam Char Character type.
 * @param chr Character code.
 * @return \b 0 if the code is greater than 0xFF, otherwise returns the same character code.
 */
template<typename Char>
    requires std::is_integral_v<Char>
constexpr auto to_ascii(Char chr) -> char
{
    return chr <= std::numeric_limits<unsigned char>::max() ? static_cast<char>(chr) : '\0';
}

} // namespace PlainCloud::Util::Unicode
