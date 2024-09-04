/**
 * @file ostream_sink.h
 * @brief Contains definition of OStreamSink class.
 */

#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sink.h" // IWYU pragma: export

#include <concepts>
#include <ostream>
#include <string_view>
#include <utility>

namespace PlainCloud::Log {

/**
 * @brief Output stream-based sink
 *
 * @tparam Logger %Logger class type intended for the sink to be used with.
 */
template<typename Logger>
//    requires(std::convertible_to<typename Logger::StringType, typename Logger::StringViewType>)
class OStreamSink : public Sink<Logger> {
public:
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::StringType;
    using typename Sink<Logger>::StringViewType;
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

    auto message(FormatBufferType& buffer, RecordType& record) -> void override
    {
        auto orig_size = buffer.size();
        this->format(buffer, record);
        buffer.push_back('\n');
        m_ostream.write(buffer.data() + orig_size, buffer.size() - orig_size);
        buffer.resize(orig_size);
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

private:
    std::basic_ostream<CharType> m_ostream;
};
} // namespace PlainCloud::Log
