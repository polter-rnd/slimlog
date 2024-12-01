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
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

namespace SlimLog {

/** @cond */
namespace Detail {

namespace Fallback {
template<typename... Args>
inline auto fopen_s(Args... /*unused*/)
{
    return std::monostate{};
};
} // namespace Fallback

struct FOpen {
    using FilePtr = std::unique_ptr<FILE, int (*)(FILE*)>;

    FOpen() = default;
    ~FOpen() = default;

    FOpen(const FOpen&) = delete;
    FOpen(FOpen&&) = delete;
    auto operator=(const FOpen&) -> FOpen& = delete;
    auto operator=(FOpen&&) noexcept -> FOpen& = delete;

    auto open(const char* filename, const char* mode) -> FilePtr
    {
        using namespace Fallback;
        using namespace std;
        m_filename = filename;
        m_mode = mode;
        return handle(fopen_s(&m_fp, m_filename, m_mode));
    }

protected:
    auto handle(int /*unused*/) -> FilePtr
    {
        return {m_fp, std::fclose};
    }

    template<typename T = std::monostate>
    auto handle(T /*unused*/) -> FilePtr
    {
        return {std::fopen(m_filename, m_mode), std::fclose};
    }

private:
    FILE* m_fp = nullptr;
    const char* m_filename = nullptr;
    const char* m_mode = nullptr;
};

} // namespace Detail
/** @endcond */

template<typename Logger>
auto FileSink<Logger>::open(std::string_view filename) -> void
{
    m_fp = Detail::FOpen().open(std::string(filename).c_str(), "w+");
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
