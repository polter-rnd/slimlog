/**
 * @file callback_sink.h
 * @brief Contains declaration of CallbackSink class.
 */

#pragma once

#include "slimlog/location.h"
#include "slimlog/record.h"
#include "slimlog/sink.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace SlimLog {

/**
 * @brief Callback-based sink.
 *
 * This sink calls a callback function to handle log messages.
 *
 * @tparam Char Character type for the string.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename Char,
    std::size_t BufferSize = DefaultSinkBufferSize,
    typename Allocator = std::allocator<Char>>
class CallbackSink : public FormattableSink<Char, BufferSize, Allocator> {
public:
    using typename FormattableSink<Char, BufferSize, Allocator>::RecordType;
    using typename FormattableSink<Char, BufferSize, Allocator>::StringViewType;
    using typename FormattableSink<Char, BufferSize, Allocator>::FormatBufferType;

    /**
     * @brief Log callback type.
     *
     * @param level The log level.
     * @param location The log location (file, line, function).
     * @param message The formatted log message (guaranteed to be null-terminated).
     */
    using LogCallback = std::function<void(Level level, Location location, StringViewType message)>;

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
        : FormattableSink<Char, BufferSize, Allocator>(std::forward<Args>(args)...)
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
    auto message(const RecordType& record) -> void override;

    /**
     * @brief Flushes the output stream.
     */
    auto flush() -> void override;

private:
    LogCallback m_callback;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sinks/callback_sink-inl.h" // IWYU pragma: keep
#endif
