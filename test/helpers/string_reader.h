#pragma once

#include <array>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

class StringReader {
public:
    explicit StringReader(std::istream& stream)
        : m_stream(stream)
    {
    }

    StringReader(const StringReader& other) = delete;
    auto operator=(const StringReader& other) -> StringReader& = delete;
    StringReader(StringReader&& other) noexcept = delete;
    auto operator=(StringReader&& other) noexcept -> StringReader& = delete;

    virtual ~StringReader() noexcept = default;

    [[nodiscard]] virtual auto read() const -> std::string
    {
        m_stream.sync();
        return {std::istreambuf_iterator<char>(m_stream), std::istreambuf_iterator<char>()};
    }

    auto operator()() const -> std::string
    {
        return read();
    }

    auto operator==(std::string_view str) const -> bool
    {
        return str == read();
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
    std::istream& m_stream;
};
