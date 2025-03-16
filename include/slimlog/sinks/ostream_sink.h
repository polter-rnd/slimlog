/**
 * @file ostream_sink.h
 * @brief Contains declaration of OStreamSink class.
 */

#pragma once

#include "slimlog/sink.h" // IWYU pragma: export
#include "slimlog/util/types.h"

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
 * @tparam String String type for log messages.
 * @tparam Char Character type for the string.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename String,
    typename Char = Util::Types::UnderlyingCharType<String>,
    std::size_t BufferSize = DefaultSinkBufferSize,
    typename Allocator = std::allocator<Char>>
class OStreamSink : public FormattableSink<String, Char, BufferSize, Allocator> {
public:
    using typename FormattableSink<String, Char, BufferSize, Allocator>::RecordType;
    using typename FormattableSink<String, Char, BufferSize, Allocator>::FormatBufferType;

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
        : FormattableSink<String, Char, BufferSize, Allocator>(std::forward<Args>(args)...)
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
        : FormattableSink<String, Char, BufferSize, Allocator>(std::forward<Args>(args)...)
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
    auto message(RecordType& record) -> void override;

    /**
     * @brief Flushes the output stream.
     */
    auto flush() -> void override;

private:
    std::basic_ostream<Char>& m_ostream;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sinks/ostream_sink-inl.h" // IWYU pragma: keep
#endif
