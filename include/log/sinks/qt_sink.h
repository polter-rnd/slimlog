#pragma once

#include "log/level.h"
#include "log/location.h"

#include <log/sink.h>

#include <QDebug>
#include <QString>

namespace PlainCloud::Log {

template<>
auto from_std_string(const std::string& str) -> QString
{
    return QString::fromStdString(str);
}

template<>
auto from_std_wstring(const std::wstring& str) -> QString
{
    return QString::fromWCharArray(str.data(), str.size());
}

template<typename String>
class QtSink : public Sink<String> {
public:
    void message(
        const Level level,
        const String& category,
        const String& message,
        const Location& caller) const override
    {
        QString level_string;
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
        qWarning() << category << " [" << level_string << "] <" << caller.file_name() << "|"
                   << caller.function_name() << ":" << caller.line() << "> " << message;
    }

    auto flush() -> void override
    {
    }
};
} // namespace PlainCloud::Log
