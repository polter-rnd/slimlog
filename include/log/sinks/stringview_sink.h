#pragma once

#include "log/level.h"
#include "log/location.h"
#include "log/sink.h"

#include <array>
#include <concepts>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace PlainCloud::Log {

namespace {
enum class Flag { Unknown, Level, Topic, Message, File, Line, Function };

static constexpr auto Flags = []() consteval {
    std::array<Flag, std::numeric_limits<unsigned char>::max()> result;

    result.fill(Flag::Unknown);
    result['l'] = Flag::Level;
    result['t'] = Flag::Topic;
    result['m'] = Flag::Message;
    result['F'] = Flag::File;
    result['L'] = Flag::Line;
    result['f'] = Flag::Function;

    return result;
}();

template<typename>
struct Levels { };

template<>
struct Levels<char> {
    std::string_view trace = "TRACE";
    std::string_view debug = "DEBUG";
    std::string_view info = "INFO";
    std::string_view warning = "WARN";
    std::string_view error = "ERROR";
    std::string_view fatal = "FATAL";
};

template<>
struct Levels<wchar_t> {
    std::wstring_view trace = L"TRACE";
    std::wstring_view debug = L"DEBUG";
    std::wstring_view info = L"INFO";
    std::wstring_view warning = L"WARN";
    std::wstring_view error = L"ERROR";
    std::wstring_view fatal = L"FATAL";
};
} // namespace

template<typename String>
    requires(
        std::convertible_to<String, std::string_view>
        || std::convertible_to<String, std::wstring_view>)
class StringViewSink : public Sink<String> {
public:
    using CharType = typename std::remove_cvref_t<String>::value_type;

    class Pattern {
    public:
        template<typename... Args>
        Pattern(const String& pattern = {}, Args&&... args)
            : m_pattern(pattern)
        {
            set_levels({std::forward<Args>(args)...});
        }

        auto empty() const -> bool
        {
            return m_pattern.empty();
        }

        auto format(
            std::basic_stringstream<CharType>& result,
            const Level level,
            const String& category,
            const String& message,
            const Location& caller) const -> void
        {
            if (m_pattern.empty()) {
                result << message;
            } else {
                std::basic_string_view<CharType> pattern{m_pattern};

                for (;;) {
                    const auto pos = pattern.find('%');
                    const auto len = pattern.size();
                    if (pos == String::npos || pos == len - 1) {
                        result << pattern;
                        break;
                    }

                    const auto chr = pattern.at(pos + 1);
                    const auto flag = Flags[chr];
                    if (flag != Flag::Unknown) {
                        result << pattern.substr(0, pos);
                        switch (flag) {
                        case Flag::Level:
                            switch (level) {
                            case Level::Fatal:
                                result << m_levels.fatal;
                                break;
                            case Level::Error:
                                result << m_levels.error;
                                break;
                            case Level::Warning:
                                result << m_levels.warning;
                                break;
                            case Level::Info:
                                result << m_levels.info;
                                break;
                            case Level::Debug:
                                result << m_levels.debug;
                                break;
                            case Level::Trace:
                                result << m_levels.trace;
                                break;
                            }
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
                        default:
                            break;
                        }
                        pattern = pattern.substr(pos + 2);
                    } else {
                        result << pattern.substr(0, pos + 1);
                        pattern = pattern.substr(pos + (chr == '%' ? 2 : 1));
                    }
                }
            }
            result << '\n';
        }

        auto set_pattern(const String& pattern)
        {
            m_pattern = pattern;
        }

        auto set_levels(const std::initializer_list<std::pair<Level, String>>& levels)
        {
            for (const auto& level : levels) {
                switch (level.first) {
                case Level::Fatal:
                    m_levels.fatal = level.second;
                    break;
                case Level::Error:
                    m_levels.error = level.second;
                    break;
                case Level::Warning:
                    m_levels.warning = level.second;
                    break;
                case Level::Info:
                    m_levels.info = level.second;
                    break;
                case Level::Debug:
                    m_levels.debug = level.second;
                    break;
                case Level::Trace:
                    m_levels.trace = level.second;
                    break;
                }
            }
        }

    private:
        String m_pattern;
        Levels<CharType> m_levels;
    };

    template<typename... Args>
    StringViewSink(Args&&... args)
        : m_pattern(std::forward<Args>(args)...)
    {
    }

    auto set_pattern(const String& pattern) -> std::shared_ptr<Sink<String>> override
    {
        m_pattern.set_pattern(pattern);
        return this->shared_from_this();
    }

    auto set_levels(const std::initializer_list<std::pair<Level, String>>& levels)
        -> std::shared_ptr<Sink<String>> override
    {
        m_pattern.set_levels(levels);
        return this->shared_from_this();
    }

    auto format(
        std::basic_stringstream<CharType>& result,
        const Level level,
        const String& category,
        const String& message,
        const Location& caller) const -> void
    {
        m_pattern.format(result, level, category, message, caller);
    }

private:
    Pattern m_pattern;
};
} // namespace PlainCloud::Log
