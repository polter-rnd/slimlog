#pragma once

#include "slimlog/util/unicode.h"

#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
#include <fmt/args.h>
#include <fmt/chrono.h> // IWYU pragma: keep
#include <fmt/format.h> // IWYU pragma: keep
#include <fmt/xchar.h> // IWYU pragma: keep

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

/*
 * Macro to define the character types used for testing.
 * This is used to create a list of character types for template specialization.
 * The types are defined based on the presence of char8_t, char16_t, and char32_t.
 * If these types are not available, only char and wchar_t are used.
 */
#ifdef SLIMLOG_CHAR8_T
#define TEST_CHAR8_T , char8_t
#else
#define TEST_CHAR8_T
#endif
#ifdef SLIMLOG_CHAR16_T
#define TEST_CHAR16_T , char16_t
#else
#define TEST_CHAR16_T
#endif
#ifdef SLIMLOG_CHAR32_T
#define TEST_CHAR32_T , char32_t
#else
#define TEST_CHAR32_T
#endif
#define SLIMLOG_CHAR_TYPES char, wchar_t TEST_CHAR8_T TEST_CHAR16_T TEST_CHAR32_T

/**
 * @brief Creates a basic string with the specified character type from a string literal
 *
 * This helper properly handles UTF-8 input including multi-byte sequences like emojis,
 * and converts them correctly to the requested character type.
 *
 * @tparam Char The character type for the output string
 * @param str The string literal to convert (UTF-8 encoded)
 * @return A basic_string with the requested character type
 */
template<typename Char>
inline auto make_string(std::string_view str) -> std::basic_string<Char>
{
    if constexpr (std::is_same_v<Char, char>) {
        // For char, just copy the UTF-8 bytes directly
        return std::string(str);
    } else {
        // For all other types, use proper Unicode conversion
        const size_t codepoints = SlimLog::Util::Unicode::count_codepoints(str.data(), str.size());
        if (codepoints == 0) {
            return {};
        }

        // Allocate buffer with space for characters + null terminator
        std::vector<Char> buffer((std::is_same_v<Char, char8_t> ? str.size() : codepoints) + 1);

        // Convert UTF-8 string to target character type
        const auto written = SlimLog::Util::Unicode::from_multibyte(
            buffer.data(), buffer.size(), str.data(), str.size() + 1);

        // Create string from buffer (excluding null terminator)
        return std::basic_string<Char>(buffer.data(), written - 1);
    }
}

/**
 * @brief Structure for holding log message pattern fields
 * @tparam Char The character type for the pattern fields.
 */
template<typename Char>
struct PatternFields {
    std::basic_string<Char> category{};
    std::basic_string<Char> level{};
    std::basic_string<Char> file{};
    std::basic_string<Char> message{};
    std::chrono::sys_seconds time;
    std::size_t thread_id{};
    std::size_t line{};
    std::size_t nsec{};
};

/**
 * Format a log message according to the pattern with the provided fields
 * @param pattern The format pattern to use
 * @param fields The field values to insert
 * @return Formatted log string
 */
template<typename Char>
static auto pattern_format(std::basic_string_view<Char> pattern, const PatternFields<Char>& fields)
    -> std::basic_string<Char>
{
    // Use fmt::vformat with the appropriate context type for any char type
    using Appender = fmt::basic_appender<Char>;
    using FormatContext = fmt::basic_format_context<Appender, Char>;

    constexpr std::size_t MsecInNsec = 1000000;
    constexpr std::size_t UsecInNsec = 1000;

    fmt::dynamic_format_arg_store<FormatContext> args;
    args.push_back(fmt::arg(make_string<Char>("category").c_str(), fields.category));
    args.push_back(fmt::arg(make_string<Char>("level").c_str(), fields.level));
    args.push_back(fmt::arg(make_string<Char>("thread").c_str(), fields.thread_id));
    args.push_back(fmt::arg(make_string<Char>("file").c_str(), fields.file));
    args.push_back(fmt::arg(make_string<Char>("line").c_str(), fields.line));
    args.push_back(fmt::arg(make_string<Char>("message").c_str(), fields.message));
    args.push_back(fmt::arg(make_string<Char>("time").c_str(), fields.time));
    args.push_back(fmt::arg(make_string<Char>("msec").c_str(), fields.nsec / MsecInNsec));
    args.push_back(fmt::arg(make_string<Char>("usec").c_str(), fields.nsec / UsecInNsec));
    args.push_back(fmt::arg(make_string<Char>("nsec").c_str(), fields.nsec));

    return fmt::vformat(
        fmt::basic_string_view<Char>{pattern}, fmt::basic_format_args<FormatContext>{args});
}
