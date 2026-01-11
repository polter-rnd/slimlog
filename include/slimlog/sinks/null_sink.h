/**
 * @file null_sink.h
 * @brief Contains declaration of NullSink class.
 */

#pragma once

#include "slimlog/sink.h"

#include <utility>

namespace slimlog {

/**
 * @brief Null sink for testing and benchmarking.
 *
 * This sink performs no action and is intended for use in performance tests.
 *
 * @tparam Char Character type for the string.
 */
template<typename Char>
class NullSink : public Sink<Char> {
public:
    using typename Sink<Char>::RecordType;

    /**
     * @brief Constructs a new NullSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit NullSink(Args&&... args)
        : Sink<Char>(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record but does not output it.
     *
     * @param record The log record to process.
     */
    auto message(const RecordType& record) -> void override
    {
        std::ignore = record;
    }

    /**
     * @brief Flush operation (no-op for NullSink).
     */
    auto flush() -> void override
    {
    }
};

} // namespace slimlog
