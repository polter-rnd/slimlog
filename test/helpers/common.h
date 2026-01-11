#pragma once

#include "slimlog/util/unicode.h"

#include <mettle.hpp>

#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/xchar.h> // IWYU pragma: keep

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @brief Type holder for character type and threading policy combinations.
 * @tparam TChar The character type (char, wchar_t, char8_t, etc.)
 * @tparam TThreadingPolicy The threading policy (SingleThreadedPolicy or MultiThreadedPolicy)
 */
template<typename TChar, typename TThreadingPolicy>
struct CharThreading {
    using Char = TChar;
    using ThreadingPolicy = TThreadingPolicy;
};

/*
 * Macro to define the character types used for testing.
 * This is used to create a list of character types for template specialization.
 * The types are defined based on the presence of char8_t, char16_t, and char32_t.
 * If these types are not available, only char and wchar_t are used.
 */
#ifdef SLIMLOG_CHAR8_T
#define TEST_CHAR8_T , char8_t
#define TEST_CHAR8_T_THREADING                                                                     \
    , CharThreading<char8_t, SingleThreadedPolicy>, CharThreading<char8_t, MultiThreadedPolicy>
#else
#define TEST_CHAR8_T
#define TEST_CHAR8_T_THREADING
#endif
#ifdef SLIMLOG_CHAR16_T
#define TEST_CHAR16_T , char16_t
#define TEST_CHAR16_T_THREADING                                                                    \
    , CharThreading<char16_t, SingleThreadedPolicy>, CharThreading<char16_t, MultiThreadedPolicy>
#else
#define TEST_CHAR16_T
#define TEST_CHAR16_T_THREADING
#endif
#ifdef SLIMLOG_CHAR32_T
#define TEST_CHAR32_T , char32_t
#define TEST_CHAR32_T_THREADING                                                                    \
    , CharThreading<char32_t, SingleThreadedPolicy>, CharThreading<char32_t, MultiThreadedPolicy>
#else
#define TEST_CHAR32_T
#define TEST_CHAR32_T_THREADING
#endif

// clang-format off
#define SLIMLOG_CHAR_TYPES char, wchar_t TEST_CHAR8_T TEST_CHAR16_T TEST_CHAR32_T
#define SLIMLOG_CHAR_THREADING_TYPES CharThreading<char, SingleThreadedPolicy>, \
                                     CharThreading<char, MultiThreadedPolicy>, \
                                     CharThreading<wchar_t, SingleThreadedPolicy>, \
                                     CharThreading<wchar_t, MultiThreadedPolicy> \
                                     TEST_CHAR8_T_THREADING \
                                     TEST_CHAR16_T_THREADING \
                                     TEST_CHAR32_T_THREADING
// clang-format on

/**
 * @brief Returns a log filename based on the test name and character type.
 * @param test_name The name of the test.
 * @tparam Char The character type for the log filename.
 * @return A string representing the log filename.
 */
template<typename Char>
constexpr auto get_log_filename(std::string_view test_name) -> std::string
{
    auto log_filename = "test_" + std::string(test_name);
    if constexpr (std::is_same_v<Char, char>) {
        log_filename += ".char.log";
    } else if constexpr (std::is_same_v<Char, char8_t>) {
        log_filename += ".char8_t.log";
    } else if constexpr (std::is_same_v<Char, char16_t>) {
        log_filename += ".char16_t.log";
    } else if constexpr (std::is_same_v<Char, char32_t>) {
        log_filename += ".char32_t.log";
    } else if constexpr (std::is_same_v<Char, wchar_t>) {
        log_filename += ".wchar_t.log";
    } else {
        log_filename += ".unknown.log";
    }
    return log_filename;
}

/**
 * @brief Mocks the current time for testing purposes.
 *
 * This function returns a fixed time point representing
 * 2023-06-15 14:32:45 with 123456 nanoseconds.
 *
 * @return A pair containing the sys_seconds time point and nanoseconds.
 */
auto inline time_mock() -> std::pair<std::chrono::sys_seconds, std::size_t>
{
    // Return 2023-06-15 14:32:45 with 123456 nanoseconds
    constexpr auto Timestamp = 1686839565; // seconds since epoch
    constexpr auto Nanoseconds = 123456; // nanoseconds
    return {std::chrono::sys_seconds{std::chrono::seconds{Timestamp}}, Nanoseconds};
}

/**
 * @brief Creates a basic string with the specified character type from UTF-8 input.
 *
 * See @ref slimlog::util::unicode::from_utf8 for details.
 */
template<typename Char, typename T>
auto from_utf8(T&& str) -> std::basic_string<Char>
{
    return slimlog::util::unicode::from_utf8<Char>(std::forward<T>(str));
}

/**
 * @brief Returns a collection of test strings with various Unicode characters.
 *
 * This function provides a set of strings containing different types of Unicode content
 * for testing purposes, including ASCII text, Cyrillic, Chinese characters, emojis,
 * and mathematical symbols.
 *
 * @tparam Char The character type for the output strings.
 * @return A vector of basic_string objects containing Unicode test data.
 */
template<typename Char>
auto unicode_strings() -> std::vector<std::basic_string<Char>>
{
    return {
        from_utf8<Char>("Simple ASCII message"),
        from_utf8<Char>(u8"Привет, мир!"),
        from_utf8<Char>(u8"你好，世界!"),
        from_utf8<Char>(u8"Some emojis: 😀, 😁, 😂, 🤣, 😃, 😄, 😅, 😆"),
        from_utf8<Char>(u8"Mathematical symbols: 𝕄𝕒𝕥𝕙 𝔽𝕦𝕟𝕔𝕥𝕚𝕠𝕟𝕤 𝕒𝕟𝕕 𝔾𝕣𝕒𝕡𝕙𝕤 ∮")};
};

/**
 * @brief Structure for holding log message pattern fields.
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
 * Format a log message according to the pattern with the provided fields.
 * @param pattern The format pattern to use.
 * @param fields The field values to insert.
 * @return Formatted log string.
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

/**
 * @brief Helper to verify message in capturer
 */
template<template<typename> typename Capturer, typename Char>
void expect_message(
    Capturer<Char>& capturer,
    const std::basic_string<Char>& expected_message,
    const std::basic_string<Char>& pattern = from_utf8<Char>("{message}"))
{
    using namespace mettle;
    PatternFields<Char> fields;
    fields.message = expected_message;
    const auto expected_output = pattern_format<Char>(pattern, fields) + Char{'\n'};
    expect(capturer.read(), equal_to(expected_output));
}

/**
 * @brief Helper to verify no message in capturer
 */
template<template<typename> typename Capturer, typename Char>
void expect_no_message(Capturer<Char>& capturer)
{
    using namespace mettle;
    expect(capturer.read(), equal_to(std::basic_string<Char>{}));
}
