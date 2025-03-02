#pragma once

#include <sstream>
#include <string>

class OutputCapturer {
public:
    explicit OutputCapturer(std::ostream& stream)
        : m_stream(stream)
        , m_oldbuf(m_stream.rdbuf(m_capture.rdbuf()))
    {
    }

    OutputCapturer(OutputCapturer&& other) noexcept = delete;
    OutputCapturer(const OutputCapturer& other) = delete;

    auto operator=(OutputCapturer&& other) noexcept -> OutputCapturer& = delete;
    auto operator=(const OutputCapturer& other) -> OutputCapturer& = delete;

    ~OutputCapturer() noexcept
    {
        m_stream.rdbuf(m_oldbuf);
    }

    auto clear() -> void
    {
        m_capture.str("");
    }

    auto output() const -> std::string
    {
        return m_capture.str();
    }

private:
    std::stringstream m_capture;
    std::ostream& m_stream;
    std::streambuf* m_oldbuf;
};
