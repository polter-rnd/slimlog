/**
 * @file format.h
 * @brief Contains definitions of FormatString, FormatBuffer, and Format classes.
 */

#pragma once

#ifdef ENABLE_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h> // IWYU pragma: export
#else
#include <fmt/core.h> // IWYU pragma: export
#endif
#include <fmt/chrono.h> // IWYU pragma: keep
#include <fmt/format.h> // IWYU pragma: keep
#include <fmt/xchar.h> // IWYU pragma: keep
#else
#include <format>
#endif

#include "location.h"
#include "util/buffer.h"
#include "util/types.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace PlainCloud::Log {

#ifdef ENABLE_FMTLIB
/**
 * @brief Alias for \a fmt::basic_format_string.
 *
 * @tparam T Character type.
 * @tparam Args Format argument types.
 */
template<typename T, typename... Args>
using FormatString = fmt::basic_format_string<T, Args...>;
/** @brief Alias for \a fmt::format_error. */
using FormatError = fmt::format_error;
#else
/**
 * @brief Alias for std::basic_format_string.
 *
 * @tparam T Character type.
 * @tparam Args Format argument types.
 */
template<typename T, typename... Args>
using FormatString = std::basic_format_string<T, Args...>;
/** @brief Alias for \a std::format_error. */
using FormatError = std::format_error;
#endif

/**
 * @brief Wrapper class consisting of a format string and location.
 *
 * This class allows passing location as a default constructor argument
 * in a template parameter pack (see Logger::emit).
 *
 * @tparam Char Character type of the format string (`char` or `wchar_t`).
 * @tparam Args Format argument types. Should be specified explicitly.
 *
 * @note This class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class, explicitly
 *       moving access functions to the public part of a base class.
 */
template<typename Char, typename... Args>
class Format final {
public:
    /**
     * @brief Constructs a new Format object from a format string and location.
     *
     * @tparam T Format string type. Deduced from the argument.
     * @param fmt Format string.
     * @param loc Source code location.
     */
    template<typename T>
        requires(std::constructible_from<FormatString<Char, Args...>, T>
                 || std::same_as<std::decay_t<T>, const Char*>
                 || std::same_as<std::decay_t<T>, Char*>)
    // NOLINTNEXTLINE(*-explicit-conversions)
    consteval Format(T fmt, Location loc = Location::current()) // cppcheck-suppress passedByValue
        : m_fmt(std::move(fmt))
        , m_loc(loc)
    {
    }

    /** @brief Copy constructor. */
    Format(const Format&) = default;
    /** @brief Move constructor. */
    Format(Format&&) noexcept = default;
    /** @brief Destructor. */
    ~Format() = default;

    /** @brief Assignment operator. */
    auto operator=(const Format&) -> Format& = default;
    /** @brief Move assignment operator. */
    auto operator=(Format&&) noexcept -> Format& = default;

    /**
     * @brief Gets the format string.
     *
     * @return The format string.
     */
    [[nodiscard]] constexpr auto fmt() const -> const auto&
    {
        return m_fmt;
    }

    /**
     * @brief Gets the location.
     *
     * @return The location.
     */
    [[nodiscard]] constexpr auto loc() const -> const auto&
    {
        return m_loc;
    }

private:
    FormatString<Char, Args...> m_fmt;
    Location m_loc;
};

/**
 * @brief Buffer used for log message formatting.
 *
 * @tparam Char Underlying character type for the string.
 * @tparam BufferSize Size of the buffer.
 * @tparam Allocator Allocator for the buffer data.
 */
template<typename Char, std::size_t BufferSize, typename Allocator = std::allocator<Char>>
class FormatBuffer final : public MemoryBuffer<Char, BufferSize, Allocator> {
public:
    using MemoryBuffer<Char, BufferSize, Allocator>::MemoryBuffer;

    /**
     * @brief Formats a log message with compile-time argument checks.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    auto format(FormatString<Char, std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
#ifdef ENABLE_FMTLIB
        if constexpr (std::is_same_v<Char, char>) {
            fmt::format_to(fmt::appender(*this), std::move(fmt), std::forward<Args>(args)...);
        } else {
#if FMT_VERSION < 110000
            fmt::format_to(
                std::back_inserter(*this),
                static_cast<fmt::basic_string_view<Char>>(std::move(fmt)),
                std::forward<Args>(args)...);
#else
            fmt::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
#endif
        }
#else
        std::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
#endif
    }

    /**
     * @brief Formats a log message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     * @throws FormatError if fmt is not a valid format string for the provided arguments.
     */
    template<typename... Args>
    auto format_runtime(std::basic_string_view<Char> fmt, Args&... args) -> void
    {
#ifdef ENABLE_FMTLIB
        if constexpr (std::is_same_v<Char, char>) {
            fmt::vformat_to(fmt::appender(*this), std::move(fmt), fmt::make_format_args(args...));
        } else {

            fmt::vformat_to(
                std::back_inserter(*this),
                std::move(fmt),
#if FMT_VERSION < 110000
                fmt::make_format_args<fmt::buffer_context<Char>>(args...)
#else
                fmt::make_format_args<fmt::buffered_context<Char>>(args...)
#endif
            );
        }
#else
        if constexpr (std::is_same_v<Char, char>) {
            std::vformat_to(
                std::back_inserter(*this), std::move(fmt), std::make_format_args(args...));
        } else if constexpr (std::is_same_v<Char, wchar_t>) {
            std::vformat_to(
                std::back_inserter(*this), std::move(fmt), std::make_wformat_args(args...));
        } else {
            static_assert(
                Util::Types::AlwaysFalse<Char>{},
                "std::vformat_to() supports only `char` or `wchar_t` character types");
        }
#endif
    }
};

} // namespace PlainCloud::Log
