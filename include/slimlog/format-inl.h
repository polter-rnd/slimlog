/**
 * @file format-inl.h
 * @brief Contains definitions of FormatValue and CachedFormatter classes.
 */

#pragma once

// IWYU pragma: private, include "slimlog/format.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/format.h" // IWYU pragma: associated

#ifdef SLIMLOG_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
// IWYU pragma: no_include <chrono>
#if FMT_VERSION < 110000
#include <iterator>
#endif
#else
#include <array>
#endif

namespace SlimLog {

#ifndef SLIMLOG_FMTLIB
template<typename T, Formattable<T> Char>
FormatValue<T, Char>::FormatValue(const CachedFormatter<T, Char>& formatter, T value)
    : m_formatter(formatter)
    , m_value(std::move(value))
{
}

template<typename T, Formattable<T> Char>
template<typename Context>
auto FormatValue<T, Char>::format(Context& context) const
{
    return m_formatter.format(m_value, context);
}
#endif

template<typename T, Formattable<T> Char>
CachedFormatter<T, Char>::CachedFormatter(std::basic_string_view<Char> fmt)
#ifdef SLIMLOG_FMTLIB
    : m_empty(fmt.empty())
#endif
{
    FormatParseContext<Char> parse_context(std::move(fmt));
    // Suppress buggy GCC warning on fmtlib sources
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    Formatter<T, Char>::parse(parse_context);
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

template<typename T, Formattable<T> Char>
CachedFormatter<T, Char>::CachedFormatter(CachedFormatter&& other) noexcept
    : Formatter<T, Char>(other)
    , m_version(other.m_version.load(std::memory_order_relaxed))
    , m_value(other.m_value.load(std::memory_order_relaxed))
#ifdef SLIMLOG_FMTLIB
    , m_empty(other.m_empty)
#endif
    , m_lock(std::move(other.m_lock))
    , m_buffer(std::move(other.m_buffer))
{
}

template<typename T, Formattable<T> Char>
template<typename Out>
void CachedFormatter<T, Char>::format(Out& out, T value) const
{
    while (true) {
        auto v1 = m_version.load(std::memory_order_acquire);
        if (v1 & 1) {
            Util::OS::yield_processor();
            continue; // update in progress, retry
        }

        auto cached_value = m_value.load(std::memory_order_acquire);

        // Copy buffer to a local variable
        decltype(m_buffer) local_buffer;
        local_buffer.append(m_buffer);

        auto v2 = m_version.load(std::memory_order_acquire);
        if (v1 != v2) {
            Util::OS::yield_processor();
            continue; // changed, retry
        }

        if (cached_value == value) {
            out.append(local_buffer);
            return;
        }
        break; // not found, go to slow path
    }

    // Slow path: need to format under lock
    m_lock.lock();
    m_version.fetch_add(1, std::memory_order_acq_rel); // odd: update in progress

    // Format to alternate buffer
    m_buffer.clear();
#ifdef SLIMLOG_FMTLIB
    // Shortcut for numeric types without formatting
    if constexpr (std::is_arithmetic_v<T> && std::is_integral_v<T>) {
        if (m_empty) [[likely]] {
            m_buffer.append(fmt::format_int(value));
            m_value.store(value, std::memory_order_release);
            m_version.fetch_add(1, std::memory_order_release); // even: update done
            out.append(m_buffer);
            m_lock.unlock();
            return;
        }
    }

    // For libfmt it's possible to create a custom fmt::basic_format_context
    // appending to the buffer directly, which is the most efficient way.
    using Appender = std::conditional_t<
        std::is_same_v<Char, char>,
        fmt::appender,
#if FMT_VERSION < 110000
        std::back_insert_iterator<decltype(m_buffer)>
#else
        fmt::basic_appender<Char>
#endif
        >;
    fmt::basic_format_context<Appender, Char> fmt_context(Appender(m_buffer), {});
    Formatter<T, Char>::format(value, fmt_context);
#else
    // For std::format there is no way to build a custom format context,
    // so we have to use dummy format string (empty string will be omitted),
    // and pass FormatValue with a reference to CachedFormatter as an argument.
    static constexpr std::array<Char, 3> Fmt{'{', '}', '\0'};
    m_buffer.vformat(Fmt.data(), m_buffer.make_format_args(FormatValue(*this, value)));
#endif

    // Publish new cache entry
    m_value.store(value, std::memory_order_release);
    m_version.fetch_add(1, std::memory_order_release); // even: update done

    out.append(m_buffer);
    m_lock.unlock();
}

} // namespace SlimLog
