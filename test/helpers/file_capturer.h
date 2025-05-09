#pragma once

#include "common.h"

#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>

template<typename Char>
class FileCapturer {
public:
    enum class BOM : std::uint8_t { None, UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE };

    explicit FileCapturer(const std::filesystem::path& path, bool truncate_file = true)
        : m_path(path)
    {
        if (truncate_file) {
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            const std::ofstream file(path, std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                throw std::runtime_error("Error truncating file " + path.string());
            }
        }

        // Open the file in binary mode for all character types
        m_file.open(path);
        if (!m_file.is_open()) {
            throw std::runtime_error("Error opening file " + path.string());
        }

        // Read the BOM if present
        parse_bom();
    }

    [[nodiscard]] auto path() const -> std::filesystem::path
    {
        return m_path;
    }

    [[nodiscard]] auto bom() const -> BOM
    {
        return m_bom;
    }

    auto read() -> std::basic_string<Char>
    {
        // Read the BOM if not already done
        parse_bom();

        // Make sure we get the latest file contents
        m_file.sync();

        using StreamIterator = std::istreambuf_iterator<char>;
        if constexpr (std::is_same_v<Char, char>) {
            return {StreamIterator(m_file), StreamIterator()};
        } else if constexpr (std::is_same_v<Char, char16_t>) {
            return decode_utf16();
        } else if constexpr (std::is_same_v<Char, char32_t>) {
            return decode_utf32();
        }
        return make_string<Char>(std::string{StreamIterator(m_file), StreamIterator()});
    }

    void remove_file()
    {
        m_file.close();
        std::filesystem::remove(m_path);
    }

private:
    void parse_bom()
    {
        if (m_file.tellg() != 0) {
            return;
        }

        // NOLINTBEGIN(*-magic-numbers)
        std::array<char, 4> bom_buffer = {0x7F, 0x7F, 0x7F, 0x7F};
        m_file.read(bom_buffer.data(), bom_buffer.size());

        // If we read less than 2 bytes, we can't have a BOM
        if (m_file.gcount() < 2) {
            m_file.clear();
            m_file.seekg(0);
            return;
        }

        // Check for different BOMs and set file position accordingly
        if (static_cast<std::uint8_t>(bom_buffer[0]) == 0xEF
            && static_cast<std::uint8_t>(bom_buffer[1]) == 0xBB
            && static_cast<std::uint8_t>(bom_buffer[2]) == 0xBF) {
            // UTF-8 BOM: EF BB BF
            m_file.seekg(static_cast<std::streamoff>(3));
            m_bom = BOM::UTF8;
        } else if (
            static_cast<std::uint8_t>(bom_buffer[0]) == 0xFE
            && static_cast<std::uint8_t>(bom_buffer[1]) == 0xFF) {
            // UTF-16BE BOM: FE FF
            m_file.seekg(static_cast<std::streamoff>(2));
            m_bom = BOM::UTF16BE;
        } else if (
            static_cast<std::uint8_t>(bom_buffer[0]) == 0xFF
            && static_cast<std::uint8_t>(bom_buffer[1]) == 0xFE) {
            // Check if it's UTF-16LE or UTF-32LE
            if (bom_buffer[2] == 0 && bom_buffer[3] == 0) {
                // UTF-32LE BOM: FF FE 00 00
                m_file.seekg(static_cast<std::streamoff>(4));
                m_bom = BOM::UTF32LE;
            } else {
                // UTF-16LE BOM: FF FE
                m_file.seekg(static_cast<std::streamoff>(2));
                m_bom = BOM::UTF16LE;
            }
        } else if (
            static_cast<std::uint8_t>(bom_buffer[0]) == 0x00
            && static_cast<std::uint8_t>(bom_buffer[1]) == 0x00
            && static_cast<std::uint8_t>(bom_buffer[2]) == 0xFE
            && static_cast<std::uint8_t>(bom_buffer[3]) == 0xFF) {
            // UTF-32BE BOM: 00 00 FE FF
            m_file.seekg(static_cast<std::streamoff>(4));
            m_bom = BOM::UTF32BE;
        } else {
            // No recognized BOM, revert to initial position
            m_file.seekg(0);
        }
        // NOLINTEND(*-magic-numbers)
    }

    auto decode_utf16() -> std::basic_string<Char>
    {
        if (m_bom != BOM::None && m_bom != BOM::UTF16LE && m_bom != BOM::UTF16BE) {
            throw std::runtime_error(
                "File contains BOM, but requested character type is not UTF-16");
        }

        // Direct UTF-16 conversion
        const bool swap_bytes
            = (m_bom == BOM::UTF16BE && std::endian::native == std::endian::little)
            || (m_bom == BOM::UTF16LE && std::endian::native == std::endian::big);

        std::basic_string<Char> result;
        std::array<char, 2> buffer = {0};
        while (m_file.read(buffer.data(), buffer.size())) {
            Char ch;
            if (swap_bytes) {
                ch = static_cast<std::uint16_t>(static_cast<std::uint8_t>(buffer[0]) << 8U)
                    | static_cast<std::uint8_t>(buffer[1]);
            } else {
                ch = static_cast<std::uint16_t>(static_cast<std::uint8_t>(buffer[1]) << 8U)
                    | static_cast<std::uint8_t>(buffer[0]);
            }
            result.push_back(ch);
        }
        m_file.clear();
        return result;
    }

    auto decode_utf32() -> std::basic_string<Char>
    {
        if (m_bom != BOM::None && m_bom != BOM::UTF32LE && m_bom != BOM::UTF32BE) {
            throw std::runtime_error(
                "File contains BOM, but requested character type is not UTF-16");
        }

        // Direct UTF-32 conversion
        const bool swap_bytes
            = (m_bom == BOM::UTF32BE && std::endian::native == std::endian::little)
            || (m_bom == BOM::UTF32LE && std::endian::native == std::endian::big);

        std::basic_string<Char> result;
        std::array<char, 4> buffer = {0};
        while (m_file.read(buffer.data(), buffer.size())) {
            Char ch;
            if (swap_bytes) {
                ch = static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[0]) << 24U)
                    | static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[1]) << 16U)
                    | static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[2]) << 8U)
                    | static_cast<std::uint8_t>(buffer[3]);
            } else {
                ch = static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[3]) << 24U)
                    | static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[2]) << 16U)
                    | static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[1]) << 8U)
                    | static_cast<std::uint8_t>(buffer[0]);
            }
            result.push_back(ch);
        }
        m_file.clear();
        return result;
    }

    std::filesystem::path m_path;
    std::ifstream m_file;
    BOM m_bom = BOM::None;
};
