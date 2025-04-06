#pragma once

#include <iterator>
#include <sstream>
#include <string>

template<typename Char>
class OutputCapturer : public std::basic_stringstream<Char> {
public:
    explicit OutputCapturer(std::basic_ostream<Char>& stream)
        : m_stream(stream)
        , m_buf(m_stream.rdbuf(this->rdbuf()))
    {
    }

    OutputCapturer(OutputCapturer&& other) noexcept = delete;
    OutputCapturer(const OutputCapturer& other) = delete;

    auto operator=(OutputCapturer&& other) noexcept -> OutputCapturer& = delete;
    auto operator=(const OutputCapturer& other) -> OutputCapturer& = delete;

    auto read() -> std::basic_string<Char>
    {
        this->sync();
        return {std::istreambuf_iterator<Char>(*this), std::istreambuf_iterator<Char>()};
    }

    ~OutputCapturer() noexcept override
    {
        m_stream.rdbuf(m_buf);
    }

private:
    std::basic_ostream<Char>& m_stream;
    std::basic_streambuf<Char>* m_buf;
};
