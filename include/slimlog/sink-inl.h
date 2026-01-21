/**
 * @file sink-inl.h
 * @brief Contains the definition of Sink and FormattableSink classes.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sink.h" // IWYU pragma: associated

namespace slimlog {

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>::set_time_func(
    TimeFunctionType time_func) -> void
{
    const typename ThreadingPolicy::template UniqueLock<decltype(m_mutex)> lock(m_mutex);
    m_pattern.set_time_func(time_func);
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>::set_pattern(
    StringViewType pattern) -> void
{
    const typename ThreadingPolicy::template UniqueLock<decltype(m_mutex)> lock(m_mutex);
    m_pattern.set_pattern(pattern);
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>::format(
    FormatBufferType& result, const RecordType& record) -> void
{
    const typename ThreadingPolicy::template SharedLock<decltype(m_mutex)> lock(m_mutex);
    m_pattern.template format<ThreadingPolicy>(result, record);
}

} // namespace slimlog
