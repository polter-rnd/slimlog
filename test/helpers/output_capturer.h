#pragma once

#include <memory>
#include <sstream>

class OutputCapturer : public std::stringstream {
public:
    explicit OutputCapturer(std::ostream& stream)
        : m_stream(stream)
        , m_buf(m_stream.rdbuf(rdbuf()))
    {
    }

    OutputCapturer(OutputCapturer&& other) noexcept = delete;
    OutputCapturer(const OutputCapturer& other) = delete;

    auto operator=(OutputCapturer&& other) noexcept -> OutputCapturer& = delete;
    auto operator=(const OutputCapturer& other) -> OutputCapturer& = delete;

    ~OutputCapturer() noexcept override
    {
        m_stream.rdbuf(m_buf);
    }

private:
    std::ostream& m_stream;
    std::streambuf* m_buf;
};
