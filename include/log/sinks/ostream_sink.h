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
    using typename StringViewSink<String>::Pattern;

    OStreamSink(std::basic_ostream<CharType>& ostream, const String& pattern = {})
        : m_ostream(ostream)
        , m_pattern(pattern)
    {
    }

    void message(
        const Level level,
        const String& category,
        const String& message,
        const Location& caller) const override
    {
        m_ostream
            << (m_pattern.empty() ? message : m_pattern.compile(level, category, message, caller))
            << '\n';
    }

    auto flush() -> void override
    {
        m_ostream.flush();
    }

    auto set_pattern(const String& pattern) -> void override
    {
        m_pattern.set(pattern);
    }

private:
    std::basic_ostream<CharType>& m_ostream;
    Pattern m_pattern;
};
} // namespace PlainCloud::Log
