/**
 * @file callback_sink.h
 * @brief Contains declaration of CallbackSink class.
 */

#pragma once

#include "slimlog/common.h"
#include "slimlog/location.h"
#include "slimlog/sink.h"

#include <slimlog_export.h>

#include <functional>
#include <utility>

namespace SlimLog {

/**
 * @brief Callback-based sink.
 *
 * This sink calls a callback function to handle log messages.
 *
 * @tparam Char Character type for the string.
 */
template<typename Char>
class CallbackSink : public Sink<Char> {
public:
    using typename Sink<Char>::RecordType;
    using typename Sink<Char>::StringViewType;

    /**
     * @brief Log callback type.
     *
     * @param level The log level.
     * @param location The log location (file, line, function).
     * @param message The formatted log message (guaranteed to be null-terminated).
     */
    using LogCallback
        = std::function<void(Level level, const Location& location, StringViewType message)>;

    // Disable copy and move semantics because of the reference member.
    CallbackSink(const CallbackSink&) = delete;
    CallbackSink(CallbackSink&&) noexcept = delete;
    auto operator=(const CallbackSink&) -> CallbackSink& = delete;
    auto operator=(CallbackSink&&) noexcept -> CallbackSink& = delete;
    ~CallbackSink() override = default;

    /**
     * @brief Constructs a new CallbackSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param callback The log callback function.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit CallbackSink(LogCallback callback, Args&&... args)
        : Sink<Char>(std::forward<Args>(args)...)
        , m_callback(std::move(callback))
    {
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record and writes it to the output stream.
     *
     * @param record The log record to process.
     */
    SLIMLOG_EXPORT auto message(const RecordType& record) -> void override;

    /**
     * @brief Flushes the output stream.
     */
    SLIMLOG_EXPORT auto flush() -> void override;

private:
    LogCallback m_callback;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sinks/callback_sink-inl.h" // IWYU pragma: keep
#endif
