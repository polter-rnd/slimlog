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
    , m_active(other.m_active.load(std::memory_order_relaxed))
    , m_value(other.m_value.load(std::memory_order_relaxed))
#ifdef SLIMLOG_FMTLIB
    , m_empty(other.m_empty)
#endif
    , m_lock(std::move(other.m_lock))
    , m_buffer(std::move(other.m_buffer))
{
}

template<typename T, Formattable<T> Char>
auto CachedFormatter<T, Char>::operator=(CachedFormatter&& other) noexcept -> CachedFormatter&
{
    if (this == &other) {
        return *this;
    }
    Formatter<T, Char>::operator=(other);
    this->m_active.store(other.m_active.load(std::memory_order_relaxed), std::memory_order_relaxed);
    this->m_value.store(other.m_value.load(std::memory_order_relaxed), std::memory_order_relaxed);
#ifdef SLIMLOG_FMTLIB
    this->m_empty = other.m_empty;
#endif
    this->m_lock = std::move(other.m_lock);
    this->m_buffer = std::move(other.m_buffer);
    return *this;
}

template<typename T, Formattable<T> Char>
template<typename Out>
void CachedFormatter<T, Char>::format(Out& out, T value) const
{
    std::int8_t idx = 0;
    bool needs_formatting = false;
    if (auto active = m_active.load(std::memory_order_acquire); active == -1) [[unlikely]] {
        // First initialization - always format to buffer 0
        m_value.store(value, std::memory_order_relaxed);
        needs_formatting = true;
    } else {
        // Check if value changed since initialization
        T expected = m_value.load(std::memory_order_relaxed);
        if (expected != value
            && m_value.compare_exchange_weak(expected, value, std::memory_order_relaxed))
            [[unlikely]] {
            needs_formatting = true;
            idx = static_cast<std::int8_t>(1 - active);
        } else {
            idx = active;
        }
    }

    auto& buffer = m_buffer[idx]; // NOLINT(*constant-array-index)
    if (needs_formatting) {
        m_lock.lock();
        buffer.clear();
#ifdef SLIMLOG_FMTLIB
        // Shortcut for numeric types without formatting
        if constexpr (std::is_arithmetic_v<T> && std::is_integral_v<T>) {
            if (m_empty) [[likely]] {
                buffer.append(fmt::format_int(value));
                m_active.store(idx, std::memory_order_release);
                m_lock.unlock();
                out.append(buffer);
                return;
            }
        }

        // For libfmt it's possible to create a custom fmt::basic_format_context
        // appending to the buffer directly, which is the most efficient way.
        using Appender = std::conditional_t<
            std::is_same_v<Char, char>,
            fmt::appender,
#if FMT_VERSION < 110000
            std::back_insert_iterator<decltype(buffer)>
#else
            fmt::basic_appender<Char>
#endif
            >;
        fmt::basic_format_context<Appender, Char> fmt_context(Appender(buffer), {});
        Formatter<T, Char>::format(value, fmt_context);
#else
        // For std::format there is no way to build a custom format context,
        // so we have to use dummy format string (empty string will be omitted),
        // and pass FormatValue with a reference to CachedFormatter as an argument.
        static constexpr std::array<Char, 3> Fmt{'{', '}', '\0'};
        buffer.vformat(Fmt.data(), buffer.make_format_args(FormatValue(*this, value)));
#endif
        m_active.store(idx, std::memory_order_release);
        m_lock.unlock();
    }
    out.append(buffer);
}

} // namespace SlimLog
