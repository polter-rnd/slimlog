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
    // For char, just copy the UTF-8 bytes directly
    if constexpr (std::is_same_v<Char, char>) {
        return std::string(str);
    }
    // For all other types, use proper Unicode conversion
    else {
        const size_t codepoints = SlimLog::Util::Unicode::count_codepoints(str.data(), str.size());

        if (codepoints == 0) {
            return {};
        }

        // Allocate buffer with space for characters + null terminator
        std::vector<Char> buffer(codepoints + 1);

        // Convert UTF-8 string to target character type
        SlimLog::Util::Unicode::from_multibyte(
            buffer.data(), codepoints + 1, str.data(), str.size());

        // Create string from buffer (excluding null terminator)
        return std::basic_string<Char>(buffer.data(), codepoints);
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
