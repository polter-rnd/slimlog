/**
 * @file file_sink-inl.h
 * @brief Contains definition of FileSink class.
 */

#pragma once

// IWYU pragma: private, include <slimlog/sinks/file_sink.h>

#ifndef SLIMLOG_HEADER_ONLY
#include <slimlog/sinks/file_sink.h>
#endif

#include <slimlog/logger.h>
#include <slimlog/sink.h>

#include <cerrno>
#include <cstdio>
#include <memory>
#include <string_view>
#include <system_error>

namespace SlimLog {

template<typename Logger>
auto FileSink<Logger>::open(std::string_view filename) -> void
{
    m_fp = {std::fopen(filename.data(), "w+"), std::fclose};
    if (!m_fp) {
        throw std::system_error({errno, std::system_category()}, "Error opening log file");
    }
}

template<typename Logger>
auto FileSink<Logger>::message(RecordType& record) -> void
{
    FormatBufferType buffer;
    Sink<Logger>::format(buffer, record);
    buffer.push_back('\n');
    if (const auto size = buffer.size() + 1;
        std::fwrite(buffer.data(), size, 1, m_fp.get()) != size) {
        throw std::system_error({errno, std::system_category()}, "Failed writing to log file");
    }
}

template<typename Logger>
auto FileSink<Logger>::flush() -> void
{
    if (std::fflush(m_fp.get()) != 0) {
        throw std::system_error({errno, std::system_category()}, "Failed flush to log file");
    }
}

} // namespace SlimLog
