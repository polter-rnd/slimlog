#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

class FileCapturer : public std::ifstream {
public:
    explicit FileCapturer(const std::filesystem::path& path, bool truncate_file = true)
        : m_path(path)
    {
        if (truncate_file) {
            const std::ofstream file(path, std::ios_base::out | std::ios_base::trunc);
            if (!file.is_open()) {
                throw std::runtime_error("Error truncating file " + path.string());
            }
        }

        open(path);
        if (!is_open()) {
            throw std::runtime_error("Error opening file" + path.string());
        }
        seekg(0, std::ios_base::end);
    }

    [[nodiscard]] auto path() const -> std::filesystem::path
    {
        return m_path;
    }

    auto read() -> std::string
    {
        sync();
        return {std::istreambuf_iterator<char>(*this), std::istreambuf_iterator<char>()};
    }

    void remove_file()
    {
        close();
        std::filesystem::remove(m_path);
    }

private:
    std::filesystem::path m_path;
    std::streampos m_pos;
};
