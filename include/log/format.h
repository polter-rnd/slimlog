/**
 * @file format.h
 * @brief Contains definition of BasicFormatString, BasicFormat, Format and WideFormat classes.
 */

#pragma once

#if __has_include(<format>)
#include <format> // IWYU pragma: export
#endif

#ifndef __cpp_lib_format
#include <fmt/core.h> // IWYU pragma: export
#include <fmt/format.h> // IWYU pragma: export
#include <fmt/xchar.h> // IWYU pragma: export
#endif

#include "location.h"

#include <concepts>
#include <string_view>
#include <type_traits>

namespace PlainCloud::Log {

#ifdef __cpp_lib_format
template<typename T, typename... Args>
using BasicFormatString = std::basic_format_string<T, Args...>;
#else
template<typename T, typename... Args>
using BasicFormatString = fmt::basic_format_string<T, Args...>;
#endif

/**
 * @brief Wrapper class consisting of format string and location.
 *
 * This way it is possible to pass location as default constructor argument
 * in template parameter pack (see Logger::emit).
 *
 * @tparam Char Character type of a format string (`char` or `wchar_t`)
 * @tparam Args Format argument types. Should be specified explicitly.
 *
 * @note Class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class explicitly
 *       moving access functions to public part of a base class.
 */
template<typename Char, typename... Args>
struct BasicFormat final {
public:
    /**
     * @brief Construct a new BasicFormat object from format string and location.
     *
     * @tparam T Format string type. Deduced from argument.
     */
    template<typename T>
        requires(std::constructible_from<BasicFormatString<Char, Args...>, T>
                 || std::same_as<std::decay_t<T>, const Char*>
                 || std::same_as<std::decay_t<T>, Char*>)
    // NOLINTNEXTLINE(*-explicit-conversions)
    consteval BasicFormat(const T& fmt, const Location& loc = Location::current())
        : m_fmt(fmt)
        , m_loc(loc)
    {
    }

    BasicFormat(const BasicFormat&) = delete;
    BasicFormat(BasicFormat&&) = delete;
    ~BasicFormat() = default;

    auto operator=(const BasicFormat&) -> BasicFormat& = delete;
    auto operator=(BasicFormat&&) -> BasicFormat& = delete;

    /**
     * @brief Get format string.
     *
     * @return Format string.
     */
    [[nodiscard]] constexpr auto fmt() const noexcept -> const auto&
    {
        return m_fmt;
    }

    /**
     * @brief Get location.
     *
     * @return Location.
     */
    [[nodiscard]] constexpr auto loc() const noexcept -> const auto&
    {
        return m_loc;
    }

private:
    const BasicFormatString<Char, Args...> m_fmt;
    const Location m_loc;
};

template<typename... Args>
using Format = BasicFormat<char, std::type_identity_t<Args>...>;

template<typename... Args>
using WideFormat = BasicFormat<wchar_t, std::type_identity_t<Args>...>;

} // namespace PlainCloud::Log
