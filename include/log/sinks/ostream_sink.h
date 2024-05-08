#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sinks/stringview_sink.h"

#include <ostream>

namespace PlainCloud::Log {

namespace {
template<typename>
struct AlwaysFalse : std::false_type { };
} // namespace

template<typename String>
class OStreamSink : public StringViewSink<String> {
public:
    using typename StringViewSink<String>::CharType;
    using Pattern = typename StringViewSink<String>::Pattern;

    template<typename... Args>
    OStreamSink(std::basic_ostream<CharType>& ostream, Args&&... args)
        : StringViewSink<String>(std::forward<Args>(args)...)
        , m_ostream(ostream)
    {
    }

    auto message(
        const Level level, const String& category, const String& message, const Location& caller)
        -> void override
    {
        Sink<String>::template stream_buf<CharType>() << message;
        this->message(level, category, caller);
    }

    auto message(const Level level, const String& category, const Location& caller) -> void override
    {
        auto& buffer = Sink<String>::template stream_buf<CharType>();
        this->format(buffer, level, category, caller);
        m_ostream << buffer.rdbuf();
        buffer.str(std::basic_string<CharType>{});
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

private:
    std::basic_ostream<CharType>& m_ostream;
};
} // namespace PlainCloud::Log
