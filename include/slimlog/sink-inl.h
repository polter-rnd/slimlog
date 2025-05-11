/**
 * @file sink-inl.h
 * @brief Contains the definition of Sink and FormattableSink classes.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sink.h" // IWYU pragma: associated

namespace SlimLog {

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<String, Char, BufferSize, Allocator>::set_levels(
    std::initializer_list<std::pair<Level, StringViewType>> levels) -> void
{
    m_pattern.set_levels(std::move(levels));
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<String, Char, BufferSize, Allocator>::set_pattern(StringViewType pattern)
    -> void
{
    m_pattern.set_pattern(std::move(pattern));
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<String, Char, BufferSize, Allocator>::format(
    FormatBufferType& result, RecordType& record) -> void
{
    m_pattern.format(result, record);
}

} // namespace SlimLog
