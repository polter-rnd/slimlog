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

#include <tuple>
#include <vector>

namespace SlimLog {

/** @cond */
namespace Detail {
inline auto is_tls_alive() -> bool
{
    static thread_local bool alive = true;
    struct Guard {
        Guard() = default;
        Guard(const Guard&) = delete;
        auto operator=(const Guard&) -> Guard& = delete;
        Guard(Guard&&) = delete;
        auto operator=(Guard&&) -> Guard& = delete;

        ~Guard()
        {
            alive = false;
        }
    };
    static const thread_local Guard guard;
    return alive;
}
} // namespace Detail
/** @endcond */

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
CachedFormatter<T, Char>::~CachedFormatter()
{
    if (Detail::is_tls_alive() && m_cache_index != static_cast<std::size_t>(-1)) {
        auto& cache = get_cache();
        cache[m_cache_index] = std::make_pair(T{}, FormatBuffer<Char, 32>{});
        get_slots().push_back(m_cache_index);
    }
}

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

    auto& cache = get_cache();

    if (auto& free_list = get_slots(); !free_list.empty()) {
        m_cache_index = free_list.back();
        free_list.pop_back();
    } else {
        cache.emplace_back(T{}, FormatBuffer<Char, 32>{});
        m_cache_index = cache.size() - 1;
    }
}

template<typename T, Formattable<T> Char>
CachedFormatter<T, Char>::CachedFormatter(CachedFormatter&& other) noexcept
    : Formatter<T, Char>(other)
#ifdef SLIMLOG_FMTLIB
    , m_empty(other.m_empty)
#endif
    , m_cache_index(other.m_cache_index)
{
    other.m_cache_index = static_cast<std::size_t>(-1);
}

template<typename T, Formattable<T> Char>
template<typename Out>
void CachedFormatter<T, Char>::format(Out& out, T value) const
{
    auto& cache = get_cache();
    if (cache.size() <= m_cache_index) {
        cache.resize(m_cache_index + 1);
    }
    auto& [cached_value, buffer] = cache[m_cache_index];

    // Check cache for this specific formatter instance
    if (buffer.size() != 0 && cached_value == value) {
        out.append(buffer);
        return;
    }

    // Cache miss - format value
    buffer.clear();

#ifdef SLIMLOG_FMTLIB
    // Shortcut for numeric types without formatting
    if constexpr (std::is_arithmetic_v<T> && std::is_integral_v<T>) {
        if (m_empty) [[likely]] {
            buffer.append(fmt::format_int(value));
            cached_value = value;
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
        std::back_insert_iterator<std::remove_reference_t<decltype(buffer)>>
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

    // Update cache
    cached_value = value;

    out.append(buffer);
}

template<typename T, Formattable<T> Char>
auto CachedFormatter<T, Char>::get_slots() noexcept -> auto&
{
    thread_local std::vector<std::size_t> slots;
    return slots;
}

template<typename T, Formattable<T> Char>
auto CachedFormatter<T, Char>::get_cache() noexcept -> auto&
{
    thread_local std::vector<std::pair<T, FormatBuffer<Char, 32>>> cache;
    return cache;
}

} // namespace SlimLog
