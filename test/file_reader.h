#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

class FileReader {
public:
    explicit FileReader(const std::filesystem::path& path)
        : m_file(path)
    {
        if (!m_file.is_open()) {
            throw std::runtime_error("Could not open file: " + path.string());
        }
    }

    auto read() const -> std::string
    {
        std::stringstream buffer;
        buffer << m_file.rdbuf();
        return buffer.str();
    }

    explicit operator std::string() const
    {
        return read();
    }

    auto operator==(const char* str) const -> bool
    {
        return read() == str;
    }

private:
    std::ifstream m_file;
};
