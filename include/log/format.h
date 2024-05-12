/**
 * @file format.h
 * @brief Contains definition of FormatString, FormatBuffer and Format classes.
 */

#pragma once

#ifdef ENABLE_FMTLIB
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/xchar.h>
#else
#include <format>
#endif

#include "location.h"

#include <concepts>
#include <iterator>
#include <sstream>
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
    consteval Format(const T& fmt, const Location& loc = Location::current())
        : m_fmt(fmt)
        , m_loc(loc)
    {
    }

    Format(const Format&) = delete;
    Format(Format&&) = delete;
    ~Format() = default;

    auto operator=(const Format&) -> Format& = delete;
    auto operator=(Format&&) -> Format& = delete;

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
    const FormatString<Char, Args...> m_fmt;
    const Location m_loc;
};

template<typename Char>
using Buffer = typename std::basic_stringstream<Char>;

template<typename Char>
class FormatBuffer final : public Buffer<Char> {
public:
    template<typename... Args>
    auto format(const Format<Char, std::type_identity_t<Args>...>& fmt, Args&&... args) -> void
    {
#ifdef ENABLE_FMTLIB
        if constexpr (std::is_same_v<Char, char>) {
            fmt::format_to(std::ostreambuf_iterator(*this), fmt.fmt(), std::forward<Args>(args)...);
        } else {
            fmt::format_to(
                std::ostreambuf_iterator(*this), fmt.fmt().get(), std::forward<Args>(args)...);
        }
#else
        std::format_to(std::ostreambuf_iterator(*this), fmt.fmt(), std::forward<Args>(args)...);
#endif
    }
};

} // namespace PlainCloud::Log
