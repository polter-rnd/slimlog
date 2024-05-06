#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sink.h"

#include <concepts>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace PlainCloud::Log {

template<typename String>
    requires(
        std::convertible_to<String, std::string_view>
        || std::convertible_to<String, std::wstring_view>)
class StringViewSink : public Sink<String> {
public:
    using CharType = typename std::remove_cvref_t<String>::value_type;

    class Pattern {
    public:
        enum class Flag { Level, Topic, Message, File, Line, Function };

        template<typename T = CharType>
            requires std::same_as<T, char>
        Pattern(const String& pattern)
            : m_pattern(pattern)
            , m_flags(
                  {{"level", Flag::Level},
                   {"topic", Flag::Topic},
                   {"message", Flag::Message},
                   {"file", Flag::File},
                   {"line", Flag::Line},
                   {"function", Flag::Function}})
            , m_levels(
                  {{Level::Trace, "TRACE"},
                   {Level::Debug, "DEBUG"},
                   {Level::Info, "INFO"},
                   {Level::Warning, "WARN"},
                   {Level::Error, "ERROR"},
                   {Level::Fatal, "FATAL"}})
        {
        }

        template<typename T = CharType>
            requires std::same_as<T, wchar_t>
        Pattern(const String& pattern)
            : m_pattern(pattern)
            , m_flags(
                  {{L"level", Flag::Level},
                   {L"topic", Flag::Topic},
                   {L"message", Flag::Message},
                   {L"file", Flag::File},
                   {L"line", Flag::Line},
                   {L"function", Flag::Function}})
            , m_levels(
                  {{Level::Trace, L"TRACE"},
                   {Level::Debug, L"DEBUG"},
                   {Level::Info, L"INFO"},
                   {Level::Warning, L"WARN"},
                   {Level::Error, L"ERROR"},
                   {Level::Fatal, L"FATAL"}})
        {
        }

        auto set(const String& pattern)
        {
            m_pattern = pattern;
        }

        auto empty() const -> bool
        {
            return m_pattern.empty();
        }

        auto compile(
            const Level level,
            const String& category,
            const String& message,
            const Location& caller) const -> auto
        {
            std::basic_stringstream<CharType> result;
            std::basic_string_view<CharType> pattern{m_pattern};

            for (;;) {
                auto pos = pattern.find('#');
                auto end = pattern.find('#', pos + 1);
                if (end == String::npos) {
                    result << pattern;
                    return result.str();
                }
                auto const it = m_flags.find(pattern.substr(pos + 1, end - pos - 1));
                if (it != m_flags.end()) {
                    result << pattern.substr(0, pos);
                    switch (it->second) {
                    case Flag::Level:
                        result << m_levels.at(level);
                        break;
                    case Flag::Topic:
                        result << category;
                        break;
                    case Flag::Message:
                        result << message;
                        break;
                    case Flag::File:
                        result << caller.file_name();
                        break;
                    case Flag::Line:
                        result << caller.line();
                        break;
                    case Flag::Function:
                        result << caller.function_name();
                        break;
                    }
                    pattern = pattern.substr(end + 1);
                } else {
                    result << pattern.substr(0, end);
                    pattern = pattern.substr(end);
                }
            }
            return result.str();
        }

    private:
        String m_pattern;
        const std::unordered_map<std::basic_string_view<CharType>, Flag> m_flags;
        const std::unordered_map<Level, String> m_levels;
    };
};
} // namespace PlainCloud::Log