/**
 * @file ostream_sink.h
 * @brief Contains definition of FileSink class.
 */

#pragma once

#include "log/sink.h" // IWYU pragma: export

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <utility>

namespace PlainCloud::Log {

/**
 * @brief File-based sink
 *
 * @tparam Logger %Logger class type intended for the sink to be used with.
 */
template<typename Logger>
class FileSink : public Sink<Logger> {
public:
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::FormatBufferType;
    using typename Sink<Logger>::RecordType;

    /**
     * @brief Construct a new FileSink object
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param filename Output file name.
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

    auto message(RecordType& record) -> void override
    {
        // const auto orig_size = buffer.size();
        FormatBufferType buffer;
        Sink<Logger>::format(buffer, record);
        buffer.push_back('\n');
        // std::fwrite(std::next(buffer.begin(), orig_size), buffer.size() - orig_size, 1, fp);
        std::fwrite(buffer.data(), buffer.size(), 1, fp);
        // buffer.resize(orig_size);
    }

    auto flush() -> void override
    {
        std::fflush(fp);
    }

private:
    FILE* fp = nullptr;
};
} // namespace PlainCloud::Log
