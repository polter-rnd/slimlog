#pragma once

#include <iterator>
#include <sstream>
#include <string>

template<typename Char>
class StreamCapturer : public std::basic_stringstream<Char> {
public:
    using std::basic_stringstream<Char>::basic_stringstream;

    explicit StreamCapturer(std::basic_ostream<Char>& stream)
        : m_stream(&stream)
        , m_buf(m_stream->rdbuf(this->rdbuf()))
    {
    }

    auto read() -> std::basic_string<Char>
    {
        this->sync();
        return {std::istreambuf_iterator<Char>(*this), std::istreambuf_iterator<Char>()};
    }

    ~StreamCapturer() noexcept override
    {
        if (m_stream) {
            m_stream->rdbuf(m_buf);
        }
    }

private:
    std::basic_ostream<Char>* m_stream = nullptr;
    std::basic_streambuf<Char>* m_buf = nullptr;
};
