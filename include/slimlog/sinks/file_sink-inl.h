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
#include <cstdint>
#include <string>
#include <system_error>
#include <type_traits>

namespace SlimLog {

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::open(std::string_view filename) -> void
{
#ifdef _WIN32
    FILE* fp = _fsopen(std::string(filename).c_str(), "ab", _SH_DENYWR);
    m_fp = {fp, std::fclose};
#else
    m_fp = {std::fopen(std::string(filename).c_str(), "ab"), std::fclose};
#endif
    if (!m_fp) {
        throw std::system_error({errno, std::system_category()}, "Error opening log file");
    }

    // Seek to end of file
    if (std::fseek(m_fp.get(), 0, SEEK_END) != 0) {
        throw std::system_error({errno, std::system_category()}, "Error seeking log file");
    }

    // Write BOM only if the file is empty
    if (std::ftell(m_fp.get()) == 0 && !write_bom()) {
        throw std::system_error({errno, std::system_category()}, "Error writing BOM to log file");
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::write_bom() -> bool
{
    if constexpr (
        std::is_same_v<Char, char16_t> || (sizeof(Char) == 2 && std::is_same_v<Char, wchar_t>)) {
        // UTF-16 BOM
        static constexpr std::array<std::uint8_t, 2> LeBom = {0xFF, 0xFE}; // UTF-16LE
        static constexpr std::array<std::uint8_t, 2> BeBom = {0xFE, 0xFF}; // UTF-16BE
        const auto& bom = (std::endian::native == std::endian::little) ? LeBom : BeBom;
        return std::fwrite(bom.data(), bom.size(), 1, m_fp.get()) == 1;
    } else if constexpr (
        std::is_same_v<Char, char32_t> || (sizeof(Char) == 4 && std::is_same_v<Char, wchar_t>)) {
        // UTF-32 BOM
        static constexpr std::array<std::uint8_t, 4> LeBom = {0xFF, 0xFE, 0x00, 0x00}; // UTF-32LE
        static constexpr std::array<std::uint8_t, 4> BeBom = {0x00, 0x00, 0xFE, 0xFF}; // UTF-32BE
        const auto& bom = (std::endian::native == std::endian::little) ? LeBom : BeBom;
        return std::fwrite(bom.data(), bom.size(), 1, m_fp.get()) == 1;
    } else {
        return true;
    }
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FileSink<String, Char, BufferSize, Allocator>::message(const RecordType& record) -> void
{
    FormatBufferType buffer;
    this->format(buffer, record);
    buffer.push_back(static_cast<Char>('\n'));
    if (std::fwrite(buffer.data(), buffer.size() * sizeof(Char), 1, m_fp.get()) != 1) {
        throw std::system_error({errno, std::system_category()}, "Failed writing to log file");
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
