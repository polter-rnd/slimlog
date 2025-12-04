/**
 * @file callback_sink-inl.h
 * @brief Contains definition of CallbackSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/callback_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/callback_sink.h" // IWYU pragma: associated

namespace SlimLog {

template<typename Char, std::size_t BufferSize, typename Allocator>
auto CallbackSink<Char, BufferSize, Allocator>::message(const RecordType& record) -> void
{
    FormatBufferType buffer;
    this->format(buffer, record);
    buffer.push_back('\0'); // Append null-terminator for safe use in the callback
    if (m_callback) {
        // We can safely convert record.filename and record.function to const char*
        // because it's guaranteed they were null-terminated strings initially.
        m_callback(
            record.level,
            // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
            Location::current(record.filename.data(), record.function.data(), record.line),
            {buffer.begin(), buffer.size() - 1});
    }
}

template<typename Char, std::size_t BufferSize, typename Allocator>
auto CallbackSink<Char, BufferSize, Allocator>::flush() -> void
{
    // No buffering, so nothing to flush
}

} // namespace SlimLog
