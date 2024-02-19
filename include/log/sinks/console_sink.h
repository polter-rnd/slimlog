#pragma once

#include <log/logger.h>

#include <iostream>

namespace plaincloud::Log {

namespace {
template<typename StringT, typename CharT>
consteval auto IsCharTypeEqual() -> bool
{
    return (std::is_array_v<std::remove_cvref_t<StringT>>
            && std::is_same_v<std::remove_all_extents<StringT>, CharT>)
        || std::is_same_v<CharT, typename std::remove_cvref_t<StringT>::value_type>;
}

template<typename StringT>
consteval auto IsRegularString() -> bool
{
    return IsCharTypeEqual<StringT, char>();
}

template<typename StringT>
consteval auto IsWideString() -> bool
{
    return IsCharTypeEqual<StringT, wchar_t>();
}
} // namespace

template<typename StringT>
class ConsoleSink : public Logger<StringT>::Sink {
public:
    void
    emit(const Log::Level level, const Log::Location& caller, const StringT& message) const override
    {
        if constexpr (IsRegularString<StringT>()) {
            std::string_view levelString;
            switch (level) {
            case Log::Level::Fatal:
                levelString = "FATAL";
                break;
            case Log::Level::Error:
                levelString = "ERROR";
                break;
            case Log::Level::Warning:
                levelString = "WARN ";
                break;
            case Log::Level::Info:
                levelString = "INFO ";
                break;
            case Log::Level::Debug:
                levelString = "DEBUG";
                break;
            case Log::Level::Trace:
                levelString = "TRACE";
                break;
            }

            std::cout << "[" << levelString << "] <" << caller.file_name() << "|"
                      << caller.function_name() << ":" << caller.line() << "> " << message
                      << std::endl;
        } else if constexpr (IsWideString<StringT>()) {
            std::wstring_view levelString;
            switch (level) {
            case Log::Level::Fatal:
                levelString = L"FATAL";
                break;
            case Log::Level::Error:
                levelString = L"ERROR";
                break;
            case Log::Level::Warning:
                levelString = L"WARN ";
                break;
            case Log::Level::Info:
                levelString = L"INFO ";
                break;
            case Log::Level::Debug:
                levelString = L"DEBUG";
                break;
            case Log::Level::Trace:
                levelString = L"TRACE";
                break;
            }
            std::wcout << "w [" << levelString << "] <" << caller.file_name() << "|"
                       << caller.function_name() << ":" << caller.line() << "> " << message
                       << std::endl;
        }
    }

    auto flush() -> void override
    {
    }
};
} // namespace plaincloud::Log
