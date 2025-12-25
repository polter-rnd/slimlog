/**
 * @file file_sink.h
 * @brief Contains declaration of FileSink class.
 */

#pragma once

#include "slimlog/common.h"
#include "slimlog/sink.h"

#include <slimlog_export.h>

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
 * @tparam Char Character type for the string.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename Char,
    std::size_t BufferSize = DefaultSinkBufferSize,
    typename Allocator = std::allocator<Char>>
class FileSink : public FormattableSink<Char, BufferSize, Allocator> {
public:
    using typename FormattableSink<Char, BufferSize, Allocator>::RecordType;
    using typename FormattableSink<Char, BufferSize, Allocator>::FormatBufferType;

    /**
     * @brief Constructs a new FileSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param filename Path to the log file.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit FileSink(std::string_view filename, Args&&... args)
        : FormattableSink<Char, BufferSize, Allocator>(std::forward<Args>(args)...)
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
    SLIMLOG_EXPORT auto message(const RecordType& record) -> void override;

    /**
     * @brief Flushes the output stream.
     */
    SLIMLOG_EXPORT auto flush() -> void override;

protected:
    /**
     * @brief Opens a particular log file for append.
     *
     * @param filename Log file name.
     */
    SLIMLOG_EXPORT auto open(std::string_view filename) -> void;

    /**
     * @brief Writes a BOM (Byte Order Mark) to the log file.
     *
     * @return true if the BOM was written successfully, false otherwise.
     */
    SLIMLOG_EXPORT auto write_bom() -> bool;

private:
    std::unique_ptr<FILE, int (*)(FILE*)> m_fp = {nullptr, nullptr};
};
} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sinks/file_sink-inl.h" // IWYU pragma: keep
#endif
