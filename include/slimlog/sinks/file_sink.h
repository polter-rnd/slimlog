/**
 * @file file_sink.h
 * @brief Contains declaration of FileSink class.
 */

#pragma once

#include "slimlog/sink.h" // IWYU pragma: export
#include "slimlog/util/types.h"

#include <cstdio>
#include <memory>
#include <string_view>
#include <utility>

namespace SlimLog {

/**
 * @brief Output file-based sink.
 *
 * This sink writes formatted log messages directly to a file.
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
class FileSink : public FormattableSink<String, Char, BufferSize, Allocator> {
public:
    using typename FormattableSink<String, Char, BufferSize, Allocator>::RecordType;
    using typename FormattableSink<String, Char, BufferSize, Allocator>::FormatBufferType;

    /**
     * @brief Constructs a new FileSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param filename Path to the log file.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit FileSink(std::string_view filename, Args&&... args)
        : FormattableSink<String, Char, BufferSize, Allocator>(std::forward<Args>(args)...)
    {
        open(filename);
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

protected:
    /**
     * @brief Opens a particular log file for append.
     *
     * @param filename Log file name.
     */
    auto open(std::string_view filename) -> void;

private:
    std::unique_ptr<FILE, int (*)(FILE*)> m_fp = {nullptr, nullptr};
};
} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sinks/file_sink-inl.h" // IWYU pragma: keep
#endif
