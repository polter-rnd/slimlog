#pragma once

#include <sstream>
#include <string>

class StringReader {
public:
    explicit StringReader(std::stringstream& stream)
        : m_stream(stream)
    {
    }

    StringReader(StringReader&& other) noexcept = delete;
    StringReader(const StringReader& other) = delete;

    auto operator=(StringReader&& other) noexcept -> StringReader& = delete;
    auto operator=(const StringReader& other) -> StringReader& = delete;

    virtual ~StringReader() noexcept = default;

    [[nodiscard]] virtual auto read() const -> std::string
    {
        const auto result = m_stream.str();
        m_stream.str("");
        return result;
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
    std::stringstream& m_stream;
};
