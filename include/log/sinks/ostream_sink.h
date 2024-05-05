#pragma once

#include "log/level.h"
#include "log/location.h"

#include <log/sink.h>

#include <ostream>
#include <string_view>
#include <type_traits>

namespace PlainCloud::Log {

template<typename>
struct AlwaysFalse : std::false_type { };

namespace {
template<typename T>
struct Underlying {
    static_assert(AlwaysFalse<T>{}, "Not a supported string, array or pointer to integral type.");
};

template<typename T>
    requires std::is_integral_v<typename std::remove_cvref_t<T>::value_type>
struct Underlying<T> {
    using type = typename std::remove_cvref_t<T>::value_type;
};

template<typename T>
    requires std::is_array_v<std::remove_cvref_t<T>>
struct Underlying<T> {
    using type = typename std::remove_cvref_t<typename std::remove_all_extents_t<T>>;
};

template<typename T>
    requires std::is_integral_v<typename std::remove_pointer_t<T>>
struct Underlying<T> {
    using type = typename std::remove_cvref_t<typename std::remove_pointer_t<T>>;
};

template<typename T>
using UnderlyingType = typename Underlying<T>::type;

} // namespace

template<typename String>
class OStreamSink : public Sink<String> {
public:
    using Char = UnderlyingType<String>;

    OStreamSink(std::basic_ostream<Char>& ostream)
        : m_ostream(ostream)
    {
    }

    void emit(const Level level, const String& message, const Location& caller) const override
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
        } else {
            static_assert(
                AlwaysFalse<Char>{},
                "Only T = char, wchar_t are supported as underlying string type");
        }

        m_ostream << "[" << level_string << "] <" << caller.file_name() << "|"
                  << caller.function_name() << ":" << caller.line() << "> " << message << std::endl;
    }

    auto flush() -> void override
    {
    }

private:
    std::basic_ostream<Char>& m_ostream;
};
} // namespace PlainCloud::Log
