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

using SlimLog::Util::Unicode::from_utf8;

/**
 * @brief Returns a collection of test strings with various Unicode characters
 *
 * This function provides a set of strings containing different types of Unicode content
 * for testing purposes, including ASCII text, Cyrillic, Chinese characters, emojis,
 * and mathematical symbols.
 *
 * @tparam Char The character type for the output strings
 * @return A vector of basic_string objects containing Unicode test data
 */
template<typename Char>
auto unicode_strings() -> std::vector<std::basic_string<Char>>
{
    return {
        from_utf8<Char>("Simple ASCII message"),
        from_utf8<Char>("Привет, мир!"),
        from_utf8<Char>("你好，世界!"),
        from_utf8<Char>("Some emojis: 😀, 😁, 😂, 🤣, 😃, 😄, 😅, 😆"),
        from_utf8<Char>("Mathematical symbols: 𝕄𝕒𝕥𝕙 𝔽𝕦𝕟𝕔𝕥𝕚𝕠𝕟𝕤 𝕒𝕟𝕕 𝔾𝕣𝕒𝕡𝕙𝕤 ∮")};
};

/**
 * @brief Structure for holding log message pattern fields
 * @tparam Char The character type for the pattern fields.
 */
template<typename Char>
struct PatternFields {
    std::basic_string<Char> category{};
    std::basic_string<Char> level{};
    std::basic_string<Char> function{};
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
#if FMT_VERSION < 110000
    using FormatContext = fmt::buffer_context<Char>;
#else
    using Appender = fmt::basic_appender<Char>;
    using FormatContext = fmt::basic_format_context<Appender, Char>;
#endif

    constexpr std::size_t MsecInNsec = 1000000;
    constexpr std::size_t UsecInNsec = 1000;

    fmt::dynamic_format_arg_store<FormatContext> args;
    args.push_back(fmt::arg(from_utf8<Char>("category").c_str(), fields.category));
    args.push_back(fmt::arg(from_utf8<Char>("level").c_str(), fields.level));
    args.push_back(fmt::arg(from_utf8<Char>("thread").c_str(), fields.thread_id));
    args.push_back(fmt::arg(from_utf8<Char>("function").c_str(), fields.function));
    args.push_back(fmt::arg(from_utf8<Char>("file").c_str(), fields.file));
    args.push_back(fmt::arg(from_utf8<Char>("line").c_str(), fields.line));
    args.push_back(fmt::arg(from_utf8<Char>("message").c_str(), fields.message));
    args.push_back(fmt::arg(from_utf8<Char>("time").c_str(), fields.time));
    args.push_back(fmt::arg(from_utf8<Char>("msec").c_str(), fields.nsec / MsecInNsec));
    args.push_back(fmt::arg(from_utf8<Char>("usec").c_str(), fields.nsec / UsecInNsec));
    args.push_back(fmt::arg(from_utf8<Char>("nsec").c_str(), fields.nsec));

    return fmt::vformat(
        fmt::basic_string_view<Char>{pattern}, fmt::basic_format_args<FormatContext>{args});
}
