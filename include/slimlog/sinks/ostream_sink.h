/**
 * @file ostream_sink.h
 * @brief Contains declaration of OStreamSink class.
 */

#pragma once

#include "slimlog/common.h"
#include "slimlog/sink.h"

#include <slimlog_export.h>

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

namespace SlimLog {

/**
 * @brief Output stream-based sink.
 *
 * This sink writes formatted log messages to an output stream.
 *
 * @tparam Char Character type for the string.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename Char,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultSinkBufferSize,
    typename Allocator = std::allocator<Char>>
class OStreamSink : public FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator> {
public:
    using typename FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>::RecordType;
    using typename FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>::FormatBufferType;

    // Disable copy and move semantics because of the reference member.
    OStreamSink(const OStreamSink&) = delete;
    OStreamSink(OStreamSink&&) noexcept = delete;
    auto operator=(const OStreamSink&) -> OStreamSink& = delete;
    auto operator=(OStreamSink&&) noexcept -> OStreamSink& = delete;
    ~OStreamSink() override = default;

    /**
     * @brief Constructs a new OStreamSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param ostream Reference to the output stream to be used by the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit OStreamSink(std::basic_ostream<Char>& ostream, Args&&... args)
        : FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>(std::forward<Args>(args)...)
        , m_ostream(ostream)
    {
    }

    /**
     * @brief Constructs a new OStreamSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param streambuf Pointer to the output stream buffer to be used by the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit OStreamSink(std::basic_streambuf<Char>* streambuf, Args&&... args)
        : FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>(std::forward<Args>(args)...)
        , m_ostream(streambuf)
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
    std::basic_ostream<Char>& m_ostream;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sinks/ostream_sink-inl.h" // IWYU pragma: keep
#endif
