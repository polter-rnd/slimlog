/**
 * @file file_sink-inl.h
 * @brief Contains definition of FileSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/file_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/file_sink.h" // IWYU pragma: associated

#include <array>
#include <bit>
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

    if (std::fseek(m_fp.get(), 0, SEEK_END) != 0) {
        throw std::system_error({errno, std::system_category()}, "Error seeking log file");
    }

    const auto size = std::ftell(m_fp.get());
    if (size == -1) {
        throw std::system_error({errno, std::system_category()}, "Error getting log file size");
    }

    // Write BOM only if the file is empty
    if (size == 0 && !write_bom()) {
        throw std::system_error({errno, std::system_category()}, "Error writing BOM to log file");
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::write_bom() -> bool
{
    if constexpr (
        std::is_same_v<Char, char16_t> || (std::is_same_v<Char, wchar_t> && sizeof(wchar_t) == 2)) {
        // UTF-16 BOM (or 2-byte wchar_t)
        static constexpr std::array<unsigned char, 2> LeBom = {0xFF, 0xFE}; // UTF-16LE
        static constexpr std::array<unsigned char, 2> BeBom = {0xFE, 0xFF}; // UTF-16BE
        const auto& bom = (std::endian::native == std::endian::little) ? LeBom : BeBom;
        return std::fwrite(bom.data(), bom.size(), 1, m_fp.get()) == 2;
    } else if constexpr (
        std::is_same_v<Char, char32_t> || (std::is_same_v<Char, wchar_t> && sizeof(wchar_t) == 4)) {
        // UTF-32 BOM (or 4-byte wchar_t)
        static constexpr std::array<unsigned char, 4> LeBom = {0xFF, 0xFE, 0x00, 0x00}; // UTF-32LE
        static constexpr std::array<unsigned char, 4> BeBom = {0x00, 0x00, 0xFE, 0xFF}; // UTF-32BE
        const auto& bom = (std::endian::native == std::endian::little) ? LeBom : BeBom;
        return std::fwrite(bom.data(), bom.size(), 1, m_fp.get()) == 4;
    } else if constexpr (std::is_same_v<Char, char8_t> || std::is_same_v<Char, char>) {
        // UTF-8 BOM
        static constexpr std::array<unsigned char, 3> Bom = {0xEF, 0xBB, 0xBF};
        return std::fwrite(Bom.data(), Bom.size(), 1, m_fp.get()) == 3;
    }
    return true;
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
