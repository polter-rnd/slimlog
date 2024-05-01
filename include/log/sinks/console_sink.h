#pragma once

#include "log/level.h"
#include "log/location.h"

#include <log/sink.h>

#include <iostream>
#include <string_view>
#include <type_traits>

namespace PlainCloud::Log {

template<typename StringT>
class ConsoleSink : public Sink<StringT> {
public:
    void emit(const Level level, const StringT& message, const Location& caller) const override
    {
        if constexpr (char_type_equal<char>()) {
            std::string_view level_string;
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

            std::cout << "[" << level_string << "] <" << caller.file_name() << "|"
                      << caller.function_name() << ":" << caller.line() << "> " << message
                      << std::endl;
        } else if constexpr (char_type_equal<wchar_t>()) {
            std::wstring_view level_string;
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
            std::wcout << "w [" << level_string << "] <" << caller.file_name() << "|"
                       << caller.function_name() << ":" << caller.line() << "> " << message
                       << std::endl;
        }
    }

    auto flush() -> void override
    {
    }

protected:
    template<typename CharT>
    static consteval auto char_type_equal() -> bool
    {
        return (std::is_array_v<std::remove_cvref_t<StringT>>
                && std::is_same_v<std::remove_all_extents<StringT>, CharT>)
            || std::is_same_v<CharT, typename std::remove_cvref_t<StringT>::value_type>;
    };
};
} // namespace PlainCloud::Log
