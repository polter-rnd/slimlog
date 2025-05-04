#pragma once

#include "common.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>

template<typename Char>
class FileCapturer {
public:
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
        m_file.open(path, std::ios::binary);
        if (!m_file.is_open()) {
            throw std::runtime_error("Error opening file " + path.string());
        }
    }

    [[nodiscard]] auto path() const -> std::filesystem::path
    {
        return m_path;
    }

    auto read() -> std::basic_string<Char>
    {
        // Make sure we get the latest file contents
        m_file.sync();

        // Read content in binary mode
        std::string content{
            std::istreambuf_iterator<char>(m_file), std::istreambuf_iterator<char>()};

        // Normalize newlines from \r\n to \n
        size_t start_pos = 0;
        while ((start_pos = content.find("\r\n", start_pos)) != std::string::npos) {
            content.replace(start_pos, 2, "\n");
            start_pos++;
        }

        // For char type, we can just return the normalized string
        if constexpr (std::is_same_v<Char, char>) {
            return content;
        } else {
            // Convert to the requested character type
            return make_string<Char>(content);
        }
    }

    void remove_file()
    {
        m_file.close();
        std::filesystem::remove(m_path);
    }

private:
    std::filesystem::path m_path;
    std::ifstream m_file;
};
