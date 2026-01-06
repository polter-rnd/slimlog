/**
 * @file ostream_sink-inl.h
 * @brief Contains definition of OStreamSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/ostream_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/ostream_sink.h" // IWYU pragma: associated

namespace SlimLog {

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto OStreamSink<Char, ThreadingPolicy, BufferSize, Allocator>::message(const RecordType& record)
    -> void
{
    FormatBufferType buffer;
    this->format(buffer, record);
    buffer.push_back('\n');

    const typename ThreadingPolicy::UniqueLock lock(m_mutex);
    m_ostream.write(buffer.begin(), buffer.size());
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto OStreamSink<Char, ThreadingPolicy, BufferSize, Allocator>::flush() -> void
{
    const typename ThreadingPolicy::UniqueLock lock(m_mutex);
    m_ostream.flush();
}

} // namespace SlimLog
