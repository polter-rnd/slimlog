#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sinks/stringview_sink.h"

#include <ostream>

namespace PlainCloud::Log {

template<typename Logger>
class OStreamSink : public StringViewSink<Logger> {
public:
    using typename StringViewSink<Logger>::CharType;
    using typename StringViewSink<Logger>::StringType;
    using typename StringViewSink<Logger>::Pattern;

    template<typename... Args>
    OStreamSink(std::basic_ostream<CharType>& ostream, Args&&... args)
        : StringViewSink<Logger>(std::forward<Args>(args)...)
        , m_ostream(ostream)
    {
    }

    auto message(
        Logger::StreamBuffer& buffer,
        const Level level,
        const StringType& category,
        const StringType& message,
        const Location& caller) -> void override
    {
        buffer << message;
        this->message(buffer, level, category, caller);
    }

    auto message(
        Logger::StreamBuffer& buffer,
        const Level level,
        const StringType& category,
        const Location& caller) -> void override
    {
        this->format(buffer, level, category, caller);
        m_ostream << buffer.rdbuf();
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

private:
    std::basic_ostream<CharType>& m_ostream;
};
} // namespace PlainCloud::Log
