/**
 * @file dummy_sink.h
 * @brief Contains definition of DummySink class.
 */

#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sink.h" // IWYU pragma: export

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
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::StringType;
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

    auto message(
        FormatBufferType& buffer,
        Level level,
        StringType category,
        StringType message,
        Location location) -> void override
    {
        buffer.assign(message);
        this->message(buffer, level, category, location);
    }

    auto message(FormatBufferType& buffer, Level level, StringType category, Location location)
        -> void override
    {
        this->apply_pattern(buffer, level, category, location);
        buffer.push_back('\n');
    }

    auto flush() -> void override
    {
    }
};
} // namespace PlainCloud::Log
