/**
 * @file callback_sink-inl.h
 * @brief Contains definition of CallbackSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/callback_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/callback_sink.h" // IWYU pragma: associated

namespace slimlog {

template<typename Char, typename ThreadingPolicy>
auto CallbackSink<Char, ThreadingPolicy>::message(const RecordType& record) -> void
{
    if (m_callback) {
        const typename ThreadingPolicy::UniqueLock lock(m_mutex);
        // We can safely convert record.filename and record.function to const char*
        // because they're guaranteed to contain null-terminated strings initially.
        m_callback(
            record.level,
            Location::current(
                record.filename.data(), // NOLINT(bugprone-suspicious-stringview-data-usage)
                record.function.data(), // NOLINT(bugprone-suspicious-stringview-data-usage)
                static_cast<int>(record.line)),
            {record.message.data(), record.message.size()});
    }
}

template<typename Char, typename ThreadingPolicy>
auto CallbackSink<Char, ThreadingPolicy>::flush() -> void
{
    // No buffering, so nothing to flush
}

} // namespace slimlog
