#pragma once

#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

class StringReader {
public:
    explicit StringReader(const std::istream& stream)
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
        std::stringstream buffer;
        buffer << m_stream.rdbuf();
        return buffer.str();
    }

    explicit operator std::string() const
    {
        return read();
    }

    auto operator==(const char* str) const -> bool
    {
        const auto* expected = str;
        const auto actual = read();
        if (expected == actual) {
            return true;
        }
        std::cerr << "==================================================\n";
        std::cerr << "[FAIL] Expected: \"" << escape_escape_sequences(expected) << "\"\n";
        std::cerr << "         Actual: \"" << escape_escape_sequences(actual) << "\"\n";
        return false;
    }

protected:
    [[nodiscard]] auto static escape_escape_sequences(std::string_view str) -> std::string
    {
        static constexpr std::array<std::pair<char, char>, 8> Sequences{{
            {'"', '"'},
            {'\a', 'a'},
            {'\b', 'b'},
            {'\f', 'f'},
            {'\n', 'n'},
            {'\r', 'r'},
            {'\t', 't'},
            {'\v', 'v'},
        }};

        std::string result;
        result.reserve(str.size());
        for (auto chr : str) {
            for (const auto seq : Sequences) {
                if (chr == seq.first) {
                    result.push_back('\\');
                    chr = seq.second;
                    break;
                }
            }
            result.push_back(chr);
        }
        return result;
    }

private:
    const std::istream& m_stream;
};
