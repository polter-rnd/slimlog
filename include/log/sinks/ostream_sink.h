#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sinks/stringview_sink.h"

#include <ostream>

namespace PlainCloud::Log {

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
        static thread_local std::basic_stringstream<CharType> result;
        this->format(result, level, category, message, caller);
        m_ostream << result.view();
        result.str(std::basic_string<CharType>{});
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

private:
    std::basic_ostream<CharType>& m_ostream;
};
} // namespace PlainCloud::Log
