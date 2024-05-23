#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sink.h" // IWYU pragma: export

#include <concepts>
#include <ostream>
#include <string_view>
#include <utility>

namespace PlainCloud::Log {

template<typename Logger>
    requires(std::convertible_to<
             typename Logger::StringType,
             std::basic_string_view<typename Logger::CharType>>)
class OStreamSink : public Sink<Logger> {
public:
    using typename Sink<Logger>::CharType;
    using typename Sink<Logger>::StringType;
    using typename Sink<Logger>::FormatBufferType;

    template<typename... Args>
    explicit OStreamSink(const std::basic_ostream<CharType>& ostream, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
        , m_ostream(ostream.rdbuf())
    {
    }

    template<typename... Args>
    explicit OStreamSink(std::basic_streambuf<CharType>* ostream, Args&&... args)
        : Sink<Logger>(std::forward<Args>(args)...)
        , m_ostream(ostream)
    {
    }

    auto message(
        FormatBufferType& buffer,
        const Level level,
        const StringType& category,
        const StringType& message,
        const Location& caller) -> void override
    {
        buffer.assign(message);
        this->message(buffer, level, category, caller);
    }

    auto message(
        FormatBufferType& buffer,
        const Level level,
        const StringType& category,
        const Location& caller) -> void override
    {
        this->format(buffer, level, category, caller);
        buffer.push_back('\n');
        m_ostream << buffer;
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

private:
    std::basic_ostream<CharType> m_ostream;
};
} // namespace PlainCloud::Log
