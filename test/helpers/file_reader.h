#pragma once

#include "string_reader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

class FileReader : public StringReader {
public:
    explicit FileReader(const std::filesystem::path& path)
        : StringReader(m_buf)
        , m_file(path)
    {
        if (!m_file.is_open()) {
            throw std::runtime_error("Could not open file: " + path.string());
        }
    }

    auto read() const -> std::string override
    {
        m_buf << m_file.rdbuf();
        return StringReader::read();
    }

private:
    mutable std::stringstream m_buf;
    std::ifstream m_file;
};
