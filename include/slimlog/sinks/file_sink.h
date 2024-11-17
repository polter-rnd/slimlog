/**
 * @file ostream_sink.h
 * @brief Contains definition of FileSink class.
 */

#pragma once

#include <slimlog/sink.h> // IWYU pragma: export

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <utility>

namespace SlimLog {

/**
 * @brief Output file-based sink.
 *
 * This sink writes formatted log messages directly to a file.
 *
 * @tparam Logger The logger class type intended for use with this sink.
 */
template<typename Logger>
class FileSink : public Sink<Logger> {
public:
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::FormatBufferType;
    using typename Sink<Logger>::RecordType;

    /**
     * @brief Constructs a new FileSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param ostream Reference to the output stream to be used by the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit FileSink(std::string_view filename, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
    {
        fp = std::fopen(filename.data(), "w+");
        if (!fp) {
            throw std::system_error({errno, std::system_category()}, std::strerror(errno));
        }
    }

    virtual ~FileSink()
    {
        std::fclose(fp);
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record and writes it to the output stream.
     *
     * @param record The log record to process.
     */
    auto message(RecordType& record) -> void override
    {
        FormatBufferType buffer;
        Sink<Logger>::format(buffer, record);
        buffer.push_back('\n');
        std::fwrite(buffer.data(), buffer.size(), 1, fp);
    }

    /**
     * @brief Flushes the output stream.
     */
    auto flush() -> void override
    {
        std::fflush(fp);
    }

private:
    FILE* fp = nullptr;
};
} // namespace SlimLog
