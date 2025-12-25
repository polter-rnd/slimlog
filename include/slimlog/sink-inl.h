/**
 * @file sink-inl.h
 * @brief Contains the definition of Sink and FormattableSink classes.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sink.h" // IWYU pragma: associated

namespace SlimLog {

template<typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<Char, BufferSize, Allocator>::set_time_func(TimeFunctionType time_func) -> void
{
    m_pattern.set_time_func(time_func);
}

template<typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<Char, BufferSize, Allocator>::set_pattern(StringViewType pattern) -> void
{
    m_pattern.set_pattern(pattern);
}

template<typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<Char, BufferSize, Allocator>::format(
    FormatBufferType& result, const RecordType& record) -> void
{
    m_pattern.format(result, record);
}

} // namespace SlimLog
