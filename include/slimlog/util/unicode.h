/**
 * @file unicode.h
 * @brief Provides various utility classes and functions for Unicode handling.
 */

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cwchar>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace SlimLog::Util::Unicode {

/**
 * @brief Calculates the length of a Unicode code point starting from the given pointer.
 *
 * Determines the number of code units that represent a Unicode code point
 * at the specified position in a string. For UTF-8, this means calculating
 * how many bytes are used for the current character.
 *
 * @tparam Char The character type, must be an integral type.
 * @param begin Pointer to the start of the Unicode sequence.
 * @return The length of the code point in code units.
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

        const auto chr = static_cast<std::uint8_t>(*begin);
        constexpr auto CodepointLengths = 0x3a55000000000000ULL;
        return static_cast<int>((CodepointLengths >> (2U * (chr >> 3U))) & 0x3U) + 1;
    }
}

/**
 * @brief Decodes UTF-8 byte into a codepoint.
 *
 * When decoding the first byte of a string, the caller must set the state variable to 0 (accept).
 * If, after decoding one or more bytes the state 0 (accept) is reached again,
 * then the decoded Unicode character value is available through the codep parameter.
 * If the state 1 (reject) is entered, that state will never be exited unless the caller intervenes.
 *
 * @param[in,out] state  The state of the decoding.
 * @param[in,out] codep  Codepoint (valid only if resulting state is 0).
 * @param[in] byte       Next byte to decode.
 * @return               New state.
 *
 * @note The function has been edited: a std::array is used.
 *
 * @copyright Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * @sa http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 */
inline auto utf8_decode(std::uint8_t& state, std::uint32_t& codep, const std::uint8_t byte) noexcept
    -> std::uint8_t
{
    static constexpr std::array<std::uint8_t, 400> UTF8d = {{
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 00..1F
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20..3F
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 40..5F
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 60..7F
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9, // 80..9F
        7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
        7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7, // A0..BF
        8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
        2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2, // C0..DF
        0xA, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // E0..EF
        0xB, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // F0..FF
        0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
        1,   0,   1,   1,   1,   1,   1,   0,   1,   0,   1,   1,   1,   1,   1,   1, // s1..s2
        1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1, // s3..s4
        1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,
        1,   1,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1, // s5..s6
        1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1,
        1,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1 // s7..s8
    }};

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,*-magic-numbers)
    const std::uint8_t type = UTF8d[byte];
    codep = (state != 0) ? (byte & 0x3FU) | (codep << 6U) : (0xFFU >> type) & (byte);

    const std::size_t index
        = 256U + (static_cast<std::size_t>(state) * 16U) + static_cast<std::size_t>(type);
    assert(index < UTF8d.size());
    state = UTF8d[index];
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index,*-magic-numbers)

    return state;
}

/**
 * @brief Counts the number of Unicode code points in a sequence.
 *
 * Iterates over a Unicode sequence and counts how many code points it contains.
 * This is useful for determining the length of a string in characters, rather than bytes.
 *
 * @tparam Char The character type.
 * @param begin Pointer to the start of the Unicode sequence.
 * @param len Number of code units in the sequence.
 * @return The number of Unicode code points in the sequence.
 */
template<typename Char>
constexpr auto count_codepoints(const Char* begin, std::size_t len) -> std::size_t
{
    if constexpr (sizeof(Char) != 1) {
        return len;
#ifdef __cpp_char8_t
    } else if constexpr (std::is_same_v<Char, char8_t>) {
        std::uint8_t state = 0;
        std::size_t codepoints = 0;
        std::uint32_t codepoint = 0;
        for (const auto* const end = std::next(begin, len); begin != end; std::advance(begin, 1)) {
            utf8_decode(state, codepoint, static_cast<std::uint8_t>(*begin));
            if (state == 0) {
                ++codepoints;
            } else if (state == 1) {
                throw std::runtime_error("utf8_decode(): conversion error");
            }
        }
        return codepoints;
#endif
    } else {
        std::size_t codepoints = 0;
        std::mbstate_t mb = {};
        for (const auto* const end = std::next(begin, len); begin != end; ++codepoints) {
            const auto next = std::mbrlen(begin, end - begin, &mb); // NOLINT(concurrency-mt-unsafe)
            if (next == static_cast<std::size_t>(-1)) {
                throw std::runtime_error("std::mbrlen(): conversion error");
            }
            std::advance(begin, next);
        }
        return codepoints;
    }
}

/**
 * @brief Converts a character code to its ASCII equivalent.
 *
 * If the character code is within the ASCII range (0 to 0xFF), it returns
 * the corresponding character. Otherwise, it returns the null character `'\0'`.
 *
 * @tparam Char The character type, must be an integral type.
 * @param chr The character code to convert.
 * @return The ASCII character, or `'\0'` if the code is out of range.
 */
template<typename Char>
    requires std::is_integral_v<Char>
constexpr auto to_ascii(Char chr) -> char
{
    return chr <= std::numeric_limits<unsigned char>::max() ? static_cast<char>(chr) : '\0';
}

} // namespace SlimLog::Util::Unicode
