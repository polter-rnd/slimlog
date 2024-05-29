/**
 * @file format.h
 * @brief Contains definition of FormatString, FormatBuffer and Format classes.
 */

#pragma once

#ifdef ENABLE_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h> // IWYU pragma: export
#else
#include <fmt/core.h> // IWYU pragma: export
#endif
#include <fmt/format.h> // IWYU pragma: keep
#include <fmt/xchar.h> // IWYU pragma: keep
#else
#include <format>
#endif

#include "location.h"

#include <concepts>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

namespace PlainCloud::Log {

#ifdef ENABLE_FMTLIB
template<typename T, typename... Args>
using FormatString = fmt::basic_format_string<T, Args...>;
#else
template<typename T, typename... Args>
using FormatString = std::basic_format_string<T, Args...>;
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
class Format final {
public:
    /**
     * @brief Construct a new Format object from format string and location.
     *
     * @tparam T Format string type. Deduced from argument.
     */
    template<typename T>
        requires(std::constructible_from<FormatString<Char, Args...>, T>
                 || std::same_as<std::decay_t<T>, const Char*>
                 || std::same_as<std::decay_t<T>, Char*>)
    // NOLINTNEXTLINE(*-explicit-conversions)
    consteval Format(T fmt, const Location& loc = Location::current())
        : m_fmt(std::move(fmt))
        , m_loc(loc)
    {
    }

    Format(const Format&) = default;
    Format(Format&&) noexcept = default;
    ~Format() = default;

    auto operator=(const Format&) -> Format& = default;
    auto operator=(Format&&) noexcept -> Format& = default;

    /**
     * @brief Get format string.
     *
     * @return Format string.
     */
    [[nodiscard]] constexpr auto fmt() const -> auto
    {
        return m_fmt;
    }

    /**
     * @brief Get location.
     *
     * @return Location.
     */
    [[nodiscard]] constexpr auto loc() const -> auto
    {
        return m_loc;
    }

private:
    FormatString<Char, Args...> m_fmt;
    Location m_loc;
};

template<
    typename Char,
    typename Traits = std::char_traits<Char>,
    typename Allocator = std::allocator<Char>>
class FormatBuffer final : public std::basic_string<Char, Traits, Allocator> {
public:
    using std::basic_string<Char, Traits, Allocator>::basic_string;

    template<typename... Args>
    auto format(FormatString<Char, std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
#ifdef ENABLE_FMTLIB
        if constexpr (std::is_same_v<Char, char>) {
            fmt::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
        } else {
            fmt::format_to(
                std::back_inserter(*this),
                static_cast<fmt::basic_string_view<Char>>(fmt),
                std::forward<Args>(args)...);
        }
#else
        std::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
#endif
    }
};

} // namespace PlainCloud::Log
