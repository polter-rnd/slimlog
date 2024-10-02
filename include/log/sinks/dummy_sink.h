/**
 * @file dummy_sink.h
 * @brief Contains definition of DummySink class.
 */

#pragma once

#include "log/sink.h" // IWYU pragma: export

#include <utility>

namespace PlainCloud::Log {

/**
 * @brief Dummy logger sink
 *
 * Intented to be used for benchmark and testing.
 *
 * @tparam Logger %Logger class type intended for the sink to be used with.
 */
template<typename Logger>
class DummySink : public Sink<Logger> {
public:
    using typename Sink<Logger>::RecordType;
    using typename Sink<Logger>::FormatBufferType;

    /**
     * @brief Construct a new Dummy Sink object
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit DummySink(Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
    {
    }

    auto message(FormatBufferType& buffer, RecordType& record) -> void override
    {
        const auto orig_size = buffer.size();
        this->format(buffer, record);
        buffer.push_back('\n');
        buffer.resize(orig_size);
    }

    auto flush() -> void override
    {
    }
};
} // namespace PlainCloud::Log
