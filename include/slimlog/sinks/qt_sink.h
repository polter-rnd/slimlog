/**
 * @file ostream_sink.h
 * @brief Contains declaration of QMessageLoggerSink class.
 */

#pragma once

#include "slimlog/sink.h"
#include "slimlog/util/types.h"

#include <QMessageLogger>
#include <utility>

namespace SlimLog {

/**
 * @brief Log sink integrated with QMessageLogger.
 *
 * This sink does not support formatting, use qSetMessagePattern().
 *
 * @tparam String String type for log messages.
 * @tparam Char Character type for the string.
 */
template<typename Char>
class QMessageLoggerSink : public Sink<Char> {
public:
    using typename Sink<Char>::RecordType;

    /**
     * @brief Constructs a new QMessageLoggerSink object.
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param qt_log_category Log category for QMessageLogger (optional).
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit QMessageLoggerSink(const char* qt_log_category = "default", Args&&... args)
        : Sink<Char>(std::forward<Args>(args)...)
        , m_qt_log_category(qt_log_category)
    {
    }

    /**
     * @brief Processes a log record.
     *
     * Formats the log record and writes it to the output stream.
     *
     * @param record The log record to process.
     */
    auto message(const RecordType& record) -> void override
    {
        const auto msg_logger = QMessageLogger(record.filename.data(),
                                               record.line,
                                               record.function.data(),
                                               m_qt_log_category);
        //std::visit([&msg_logger, level = record.level]<typename T>(T&& message) {
            switch(record.level) {
            case Level::Trace:
                [[fallthrough]];
            case Level::Debug:
                msg_logger.debug().nospace().noquote() << record.message;
                break;
            case Level::Info:
                msg_logger.info().nospace().noquote() << record.message;
                break;
            case Level::Warning:
                msg_logger.warning().nospace().noquote() << record.message;
                break;
            case Level::Error:
                msg_logger.critical().nospace().noquote() << record.message;
                break;
            case Level::Fatal:
                msg_logger.fatal().nospace().noquote() << record.message;
                break;
            }
        //}, record.message);
    }

    /**
     * @brief Does nothing for QMessageLoggerSink.
     */
    auto flush() -> void override
    {
    }

private:
    const char* m_qt_log_category;
};

} // namespace SlimLog
