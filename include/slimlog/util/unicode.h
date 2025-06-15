/**
 * @file unicode.h
 * @brief Provides various utility classes and functions for Unicode handling.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

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
 * Returns some other positive value if more bytes have to be read.
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
constexpr auto utf8_decode(
    std::uint8_t& state, std::uint32_t& codep, const std::uint8_t byte) noexcept -> std::uint8_t
{
    // UTF-8 DFA transition table as string literal - 400 bytes total
    // First 256 bytes: character type lookup table (indexed by byte value)
    // Next 144 bytes: state transition table (indexed by state*16 + char_type)
    //
    // Character types: 0=ASCII, 1=invalid, 2=2-byte lead, 3=3-byte lead, etc.
    // States: 0=accept, 1=reject, 2-11=intermediate states for multi-byte sequences
    //
    // Layout:
    // [0-255]:   byte -> character type mapping
    // [256-399]: state transitions (9 states * 16 types each)
    const char* utf8_dfa =
        // Bytes 0x00-0x7F: ASCII characters (type 0)
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // 00-1F
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // 20-3F
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // 40-5F
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // 60-7F
        // Bytes 0x80-0xBF: UTF-8 continuation bytes
        "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
        "\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09\x09" // 80-9F
        "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07"
        "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07" // A0-BF
        // Bytes 0xC0-0xDF: 2-byte sequence leaders
        "\x08\x08\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02"
        "\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02" // C0-DF
        // Bytes 0xE0-0xEF: 3-byte sequence leaders
        "\x0A\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x04\x03\x03" // E0-EF
        // Bytes 0xF0-0xFF: 4-byte sequence leaders and invalid
        "\x0B\x06\x06\x06\x05\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" // F0-FF
        // State transition table starts here (byte 256)
        "\x00\x01\x02\x03\x05\x08\x07\x01\x01\x01\x04\x06\x01\x01\x01\x01" // s0
        "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01" // s1
        "\x01\x00\x01\x01\x01\x01\x01\x00\x01\x00\x01\x01\x01\x01\x01\x01" // s2
        "\x01\x02\x01\x01\x01\x01\x01\x02\x01\x02\x01\x01\x01\x01\x01\x01" // s3
        "\x01\x01\x01\x01\x01\x01\x01\x02\x01\x01\x01\x01\x01\x01\x01\x01" // s4
        "\x01\x02\x01\x01\x01\x01\x01\x01\x01\x02\x01\x01\x01\x01\x01\x01" // s5
        "\x01\x01\x01\x01\x01\x01\x01\x03\x01\x03\x01\x01\x01\x01\x01\x01" // s6
        "\x01\x03\x01\x01\x01\x01\x01\x03\x01\x03\x01\x01\x01\x01\x01\x01" // s7
        "\x01\x03\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"; // s8

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index,*-magic-numbers)
    const std::uint8_t type = utf8_dfa[byte];
    codep = (state != 0) ? (byte & 0x3FU) | (codep << 6U) : (0xFFU >> type) & (byte);

    const std::size_t index
        = 256U + (static_cast<std::size_t>(state) * 16U) + static_cast<std::size_t>(type);
    if (!std::is_constant_evaluated()) {
        assert(index < 400);
    }
    state = utf8_dfa[index];
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
 *
 * @note This function assumes that the input is valid UTF-8 encoded data.
 *       If the input contains invalid sequences, it will stop counting at the first invalid byte.
 */
template<typename Char>
constexpr auto count_codepoints(const Char* begin, std::size_t len) -> std::size_t
{
    if constexpr (sizeof(Char) != 1) {
        return len;
    } else {
        std::uint8_t state = 0;
        std::size_t codepoints = 0;
        std::uint32_t codepoint = 0;
        for (const auto* const end = begin + len; begin != end; ++begin) {
            utf8_decode(state, codepoint, static_cast<std::uint8_t>(*begin));
            if (state == 0) {
                ++codepoints;
            } else if (state == 1) {
                break; // Invalid sequence, stop counting
            }
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

/**
 * @brief Writes a Unicode code point to a destination buffer.
 *
 * This function handles the conversion of Unicode code points to their proper
 * representation in different character encoding formats:
 * - For UTF-16 and Windows wchar_t (2 bytes): Handles surrogate pairs for code points
 *   above the Basic Multilingual Plane (> 0xFFFF)
 * - For UTF-32 and other wchar_t implementations: Direct code point assignment
 *
 * @tparam Char The character type of the destination buffer (char16_t, char32_t, wchar_t)
 * @param dest Pointer to the destination buffer
 * @param dest_size Total size of the destination buffer in characters
 * @param codepoint Unicode code point to write (as a 32-bit unsigned integer)
 *
 * @return Number of characters written (1 for most code points, 2 for surrogate pairs,
 *         0 if there's not enough space in the buffer)
 */
template<typename Char>
auto write_codepoint(Char* dest, std::size_t dest_size, std::uint32_t codepoint) -> std::size_t
{
    if (dest_size == 0 || dest == nullptr) {
        return 0;
    }

    if constexpr (
        std::is_same_v<Char, char16_t> || (std::is_same_v<Char, wchar_t> && sizeof(Char) == 2)) {
        // NOLINTBEGIN(*-magic-numbers)
        // For UTF-16 and 2-byte wchar_t, handle surrogate pairs for codepoints above 0xFFFF
        if (codepoint <= 0xFFFF) {
            *dest = static_cast<Char>(codepoint);
            return 1;
        }
        // Supplementary planes - needs surrogate pair
        if (dest_size > 1) {
            Char high = static_cast<Char>(0xD800 + ((codepoint - 0x10000) >> 10U));
            Char low = static_cast<Char>(0xDC00 + ((codepoint - 0x10000) & 0x3FFU));
            dest[0] = high;
            dest[1] = low;
            return 2;
        }
        // NOLINTEND(*-magic-numbers)
        return 0;
    } else {
        // For char32_t and 4-byte wchar_t, direct codepoint conversion
        *dest = static_cast<Char>(codepoint);
        return 1;
    }
}

/**
 * @brief Converts a UTF-8 string to a character sequence without null termination.
 *
 * Destination buffer has to be capable of storing at least @p dest_size characters.
 * No null terminator is added to the destination buffer.
 *
 * @tparam Char Character type of the destination buffer.
 * @tparam T Character type of the source UTF-8 data (char or char8_t).
 * @param dest Pointer to destination buffer for the converted data.
 * @param dest_size Size of the destination buffer in characters.
 * @param source Pointer to UTF-8 data to be converted.
 * @param source_size Source data size in bytes.
 * @return Number of characters written (without null terminator).
 */
template<typename Char, typename T>
    requires(std::same_as<T, char> || std::same_as<T, char8_t>)
auto from_utf8(Char* dest, std::size_t dest_size, const T* source, std::size_t source_size)
    -> std::size_t
{
    if (source == nullptr || dest == nullptr || source_size == 0 || dest_size == 0) {
        return 0;
    }

    if constexpr (std::is_same_v<Char, char> || std::is_same_v<Char, char8_t>) {
        // For char, just copy the UTF-8 bytes directly
        const std::size_t to_copy = std::min(dest_size, source_size);
        std::copy_n(source, to_copy, dest);
        return to_copy;
    } else {
        // For wchar_t, char16_t, and char32_t, decode UTF-8 directly to codepoints
        std::size_t written = 0;

        // Process UTF-8 bytes to extract codepoints
        std::uint8_t state = 0;
        std::uint32_t codepoint = 0;
        for (const T* end = source + source_size; source != end; ++source) {
            utf8_decode(state, codepoint, static_cast<std::uint8_t>(*source));
            if (state == 0) {
                // If state is 0, we have a complete codepoint
                const auto res = write_codepoint<Char>(dest, dest_size - written, codepoint);
                if (res == 0) {
                    break;
                }
                dest += res;
                written += res;
            } else if (state == 1) [[unlikely]] {
                // If state is 1, we have an invalid sequence
                break;
            }
            // For any other state value, we're in the middle of a multi-byte sequence
            // Just continue to the next byte
        }

        return written;
    }
}

/**
 * @brief Creates a basic string with the specified character type from UTF-8 input
 *
 * This helper properly handles UTF-8 input including multi-byte sequences like emojis,
 * and converts them correctly to the requested character type. Accepts both string_view
 * and u8string_view, with automatic conversion from string literals, pointers, and string objects.
 *
 * @tparam Char The character type for the output string
 * @tparam T The source character type (char or char8_t)
 * @param str The string view to convert (UTF-8 encoded)
 * @return A basic_string with the requested character type
 */
template<typename Char, typename T>
    requires(std::same_as<T, char> || std::same_as<T, char8_t>)
auto from_utf8(std::basic_string_view<T> str) -> std::basic_string<Char>
{
    if (str.empty()) {
        return {};
    }

    if constexpr (std::is_same_v<Char, char>) {
        if constexpr (std::is_same_v<T, char8_t>) {
            // If T is char8_t, we need to convert it to char
            std::string buffer(str.size(), '\0');
            std::copy(str.begin(), str.end(), buffer.begin());
            return buffer;
        } else {
            // For char, just copy the UTF-8 bytes directly
            return std::string(str);
        }
    } else {
        // Calculate destination buffer size based on target character encoding:
        // - UTF-8 (1 byte): same size as source (byte-for-byte copy)
        // - UTF-16 (2 bytes): double codepoints (potential surrogate pairs)
        // - UTF-32 (4 bytes): same as codepoints (one-to-one mapping)
        const auto dest_size = sizeof(Char) == 1
            ? str.size()
            : count_codepoints(str.data(), str.size()) * (sizeof(Char) == 2 ? 2 : 1);
        if (dest_size == 0) {
            return {};
        }

        std::basic_string<Char> buffer(dest_size, Char{});
        const auto written = from_utf8(buffer.data(), buffer.size(), str.data(), str.size());
        buffer.resize(written);
        return buffer;
    }
}

/** @overload */
template<typename Char, typename T>
auto from_utf8(T&& str) -> std::basic_string<Char>
{
    if constexpr (std::is_convertible_v<T, std::u8string_view>) {
        return from_utf8<Char>(std::u8string_view(std::forward<T>(str)));
    } else {
        return from_utf8<Char>(std::string_view(std::forward<T>(str)));
    }
}

} // namespace SlimLog::Util::Unicode
