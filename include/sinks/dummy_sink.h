/**
 * @file dummy_sink.h
 * @brief Contains definition of DummySink class.
 */

#pragma once

#include "sink.h" // IWYU pragma: export

#include <utility>

namespace SlimLog {

/**
 * @brief Dummy sink for testing and benchmarking.
 *
 * This sink performs no action and is intended for use in performance tests.
 *
 * @tparam Logger The logger class type intended for use with this sink.
 */
template<typename Logger>
class DummySink : public Sink<Logger> {
public:
    using typename Sink<Logger>::RecordType;
    using typename Sink<Logger>::FormatBufferType;

    /**
     * @brief Constructs a new DummySink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit DummySink(Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record but does not output it.
     *
     * @param record The log record to process.
     */
    auto message(RecordType& record) -> void override
    {
        FormatBufferType buffer;
        Sink<Logger>::format(buffer, record);
        buffer.push_back('\n');
    }

    /**
     * @brief Flush operation (no-op for DummySink).
     */
    auto flush() -> void override
    {
    }
};

} // namespace SlimLog
