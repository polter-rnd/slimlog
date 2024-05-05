#pragma once

#include "log/level.h"
#include "log/location.h"

#include <log/sink.h>

#include <concepts>
#include <ostream>
#include <string_view>
#include <type_traits>

namespace PlainCloud::Log {

namespace {
template<typename>
struct AlwaysFalse : std::false_type { };
} // namespace

template<typename String>
    requires(
        std::convertible_to<String, std::string_view>
        || std::convertible_to<String, std::wstring_view>)
class OStreamSink : public Sink<String> {
public:
    using Char = typename std::remove_cvref_t<String>::value_type;

    OStreamSink(std::basic_ostream<Char>& ostream)
        : m_ostream(ostream)
    {
    }

    void message(
        const Level level,
        const String& category,
        const String& message,
        const Location& caller) const override
    {
        std::basic_string_view<Char> level_string;
        if constexpr (std::is_same_v<Char, char>) {
            switch (level) {
            case Level::Fatal:
                level_string = "FATAL";
                break;
            case Level::Error:
                level_string = "ERROR";
                break;
            case Level::Warning:
                level_string = "WARN ";
                break;
            case Level::Info:
                level_string = "INFO ";
                break;
            case Level::Debug:
                level_string = "DEBUG";
                break;
            case Level::Trace:
                level_string = "TRACE";
                break;
            }
        } else if constexpr (std::is_same_v<Char, wchar_t>) {
            switch (level) {
            case Level::Fatal:
                level_string = L"FATAL";
                break;
            case Level::Error:
                level_string = L"ERROR";
                break;
            case Level::Warning:
                level_string = L"WARN ";
                break;
            case Level::Info:
                level_string = L"INFO ";
                break;
            case Level::Debug:
                level_string = L"DEBUG";
                break;
            case Level::Trace:
                level_string = L"TRACE";
                break;
            }
        } else {
            static_assert(
                AlwaysFalse<Char>{},
                "Only T = char, wchar_t are supported as underlying string type");
        }

        m_ostream << category << " [" << level_string << "] <" << caller.file_name() << "|"
                  << caller.function_name() << ":" << caller.line() << "> " << message << std::endl;
    }

    auto flush() -> void override
    {
    }

private:
    std::basic_ostream<Char>& m_ostream;
};
} // namespace PlainCloud::Log
