/**
 * @file file_sink-inl.h
 * @brief Contains definition of FileSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/file_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/file_sink.h" // IWYU pragma: associated

#include <cerrno>
#include <cwchar>
#include <string>
#include <system_error>
#include <type_traits>

namespace SlimLog {

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::open(std::string_view filename) -> void
{
#if defined(_WIN32)
    FILE* fp = _fsopen(std::string(filename).c_str(), "a", _SH_DENYWR);
    m_fp = {fp, std::fclose};
#else
    m_fp = {std::fopen(std::string(filename).c_str(), "a"), std::fclose};
#endif
    if (!m_fp) {
        throw std::system_error({errno, std::system_category()}, "Error opening log file");
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::message(RecordType& record) -> void
{
    FormatBufferType buffer;
    this->format(buffer, record);
    buffer.push_back('\n');
    if constexpr (std::is_same_v<Char, wchar_t>) {
        buffer.push_back('\0');
        if (std::fputws(buffer.data(), m_fp.get()) == EOF) {
            throw std::system_error({errno, std::system_category()}, "Failed writing to log file");
        }
    } else {
        if (std::fwrite(buffer.data(), buffer.size() * sizeof(Char), 1, m_fp.get()) != 1) {
            throw std::system_error({errno, std::system_category()}, "Failed writing to log file");
        }
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::flush() -> void
{
    if (std::fflush(m_fp.get()) != 0) {
        throw std::system_error({errno, std::system_category()}, "Failed flush to log file");
    }
}

} // namespace SlimLog
