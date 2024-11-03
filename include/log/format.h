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
#include <array>
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
/** @brief Alias for \a fmt::formatter. */
template<typename T, typename Char>
using Formatter = fmt::formatter<T, Char>;
/** @brief Alias for \a fmt::basic_format_parse_context. */
template<typename Char>
using FormatParseContext = fmt::basic_format_parse_context<Char>;
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
/** @brief Alias for \a std::formatter. */
template<typename T, typename Char>
using Formatter = std::formatter<T, Char>;
/** @brief Alias for \a std::basic_format_parse_context. */
template<typename Char>
using FormatParseContext = std::basic_format_parse_context<Char>;
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

template<typename T, typename Char>
class CachedFormatter;

/**
 * @brief Wrapper for formatting a value with a cached formatter.
 *
 * Stores the value and reference to the cached formatter (see CachedFormatter).
 * Used in combination with `std::formatter<FormatValue<T, Char>, Char>` specialization.
 *
 * @tparam T Value type.
 * @tparam Char Output character type.
 */
template<typename T, typename Char>
class FormatValue final {
public:
    /**
     * @brief Constructs a new FormatValue object from a reference to the formatter and value.
     *
     * @param formatter Reference to the formatter.
     * @param value Value to be formatted.
     */
    constexpr FormatValue(const CachedFormatter<T, Char>& formatter, T value)
        : m_formatter(formatter)
        , m_value(std::move(value))
    {
    }

    ~FormatValue() = default;

    FormatValue(const FormatValue&) = delete;
    FormatValue(FormatValue&&) = delete;
    auto operator=(const FormatValue&) -> FormatValue& = delete;
    auto operator=(FormatValue&&) noexcept -> FormatValue& = delete;

    /**
     * @brief Formats the value with the cached formatter.
     *
     * @tparam Context Format context type (`std::format_context`).
     * @param context Format context.
     * @return Output iterator to the resulting buffer.
     */
    template<typename Context>
    auto format(Context& context) const
    {
        return m_formatter.format(m_value, context);
    }

private:
    const CachedFormatter<T, Char>& m_formatter;
    T m_value;
};

/**
 * @brief Wrapper to parse the format string and store the format context.
 *
 * Used to parse format string only once and then cache the formatter.
 *
 * @tparam T Value type.
 * @tparam Char Output character type.
 */
template<typename T, typename Char>
class CachedFormatter final : Formatter<T, Char> {
public:
    using Formatter<T, Char>::format;

    /**
     * @brief Constructs a new CachedFormatter object from a format string.
     *
     * @param fmt Format string.
     */
    constexpr explicit CachedFormatter(std::basic_string_view<Char> fmt)
#ifdef ENABLE_FMTLIB
        : m_empty(fmt.empty())
#endif
    {
        FormatParseContext<Char> parse_context(std::move(fmt));
        Formatter<T, Char>::parse(parse_context);
    }

    /**
     * @brief Formats the value and writes to the output buffer.
     *
     * @tparam Out Output buffer type (see MemoryBuffer).
     * @param out Output buffer.
     * @param value Value to be formatted.
     */
    template<typename Out>
    void format(Out& out, T value) const
    {
#ifdef ENABLE_FMTLIB
        if constexpr (std::is_arithmetic_v<T>) {
            if (m_empty) [[likely]] {
                out.append(fmt::format_int(value));
                return;
            }
        }

        using Appender = std::conditional_t<
            std::is_same_v<Char, char>,
            fmt::appender,
            std::back_insert_iterator<std::remove_cvref_t<Out>>>;
        fmt::basic_format_context<Appender, Char> fmt_context(Appender(out), {});
        Formatter<T, Char>::format(std::move(value), fmt_context);
#else
        static constexpr std::array<Char, 3> Fmt{'{', '}', '\0'};
        out.format(Fmt.data(), FormatValue(*this, std::move(value)));
#endif
    }

#ifdef ENABLE_FMTLIB
private:
    bool m_empty;
#endif
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
     * @brief Appends data to the end of the buffer.
     *
     * @tparam ContiguousRange Type of the source object.
     *
     * @param range Source object containing data to be added to the buffer.
     */
    template<typename ContiguousRange>
    void append(const ContiguousRange& range) // cppcheck-suppress duplInheritedMember
    {
        append(range.data(), std::next(range.data(), range.size()));
    }

    /**
     * @brief Appends data to the end of the buffer.
     *
     * @tparam U Input data type.
     * @param begin Begin input iterator.
     * @param end End input iterator.
     */
    template<typename U>
    void append(const U* begin, const U* end) // cppcheck-suppress duplInheritedMember
    {
        const auto buf_size = this->size();
        const auto count = Util::Types::to_unsigned(end - begin);
        this->resize(buf_size + count);
        std::uninitialized_copy_n(begin, count, std::next(this->begin(), buf_size));
    }

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
            fmt::format_to(
                std::back_inserter(*this),
#if FMT_VERSION < 110000
                static_cast<fmt::basic_string_view<Char>>(std::move(fmt)),
#else
                std::move(fmt),
#endif
                std::forward<Args>(args)...);
        }
#else
        std::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
#endif
    }
};

} // namespace PlainCloud::Log

#ifndef ENABLE_FMTLIB
/** @cond */
template<typename T, typename Char>
struct std::formatter<PlainCloud::Log::FormatValue<T, Char>, Char> { // NOLINT (cert-dcl58-cpp)
    constexpr auto parse(PlainCloud::Log::FormatParseContext<Char>& context)
    {
        return context.end();
    }

    template<typename Context>
    auto format(const PlainCloud::Log::FormatValue<T, Char>& wrapper, Context& context) const
    {
        return wrapper.format(context);
    }
};
/** @endcond */
#endif
