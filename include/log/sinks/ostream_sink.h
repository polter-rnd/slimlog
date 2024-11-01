/**
 * @file ostream_sink.h
 * @brief Contains definition of OStreamSink class.
 */

#pragma once

#include "log/sink.h" // IWYU pragma: export

#include <ostream>
#include <utility>

namespace PlainCloud::Log {

/**
 * @brief Output stream-based sink
 *
 * @tparam Logger %Logger class type intended for the sink to be used with.
 */
template<typename Logger>
class OStreamSink : public Sink<Logger> {
public:
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::FormatBufferType;
    using typename Sink<Logger>::RecordType;

    /**
     * @brief Construct a new OStreamSink object
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param ostream Reference to output stream to be used for the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit OStreamSink(const std::basic_ostream<CharType>& ostream, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
        , m_ostream(ostream.rdbuf())
    {
    }

    /**
     * @brief Construct a new OStreamSink object
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param streambuf Pointer to output stream buffer to be used for the sink.
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit OStreamSink(std::basic_streambuf<CharType>* streambuf, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
        , m_ostream(streambuf)
    {
    }

    auto message(RecordType& record) -> void override
    {
        FormatBufferType buffer;
        Sink<Logger>::format(buffer, record);
        buffer.push_back('\n');
        m_ostream.write(buffer.begin(), buffer.size());
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

private:
    std::basic_ostream<CharType> m_ostream;
};
} // namespace PlainCloud::Log
