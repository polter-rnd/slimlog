/**
 * @file logger.h
 * @brief Contains the definition of the Logger class.
 */

#pragma once

#include "format.h"
#include "level.h"
#include "location.h"
#include "policy.h"
#include "sink.h"
#include "util/types.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace PlainCloud::Log {

/** @brief Default buffer size for log messages. */
static constexpr auto DefaultBufferSize = 1024U;

/**
 * @brief A logger front-end class.
 *
 * Performs log message filtering and emitting through specified sinks.
 * Sink objects represent back-ends used for emitting the message.
 *
 * Usage example:
 * @code
 * Log::Logger log("main");
 * log.add_sink<Log::OStreamSink>(std::cerr, "(%t) [%l] %F|%L: %m");
 * log.info("Hello {}!", "World");
 * @endcode
 *
 * @tparam String String type for logging messages (e.g., `std::string`).
 *                Can be deduced from logger name.
 * @tparam Char Underlying char type for the string.
 *              Deduced automatically for standard C++ string types and for plain C strings.
 * @tparam ThreadingPolicy Threading policy used for operating over sinks and log level
 *                         (e.g., SingleThreadedPolicy or MultiThreadedPolicy).
 * @tparam StaticBufferSize Size of internal pre-allocated buffer. Defaults to 4096 bytes.
 */
template<
    typename String,
    typename Char = Util::Types::UnderlyingCharType<String>,
    typename ThreadingPolicy = MultiThreadedPolicy<>,
    std::size_t StaticBufferSize = DefaultBufferSize>
class Logger {
public:
    /** @brief String type for log messages. */
    using StringType = String;
    /** @brief Char type for log messages. */
    using CharType = Char;
    /** @brief String view type for log category. */
    using StringViewType = std::basic_string_view<CharType>;
    /** @brief Size of internal pre-allocated buffer. */
    static constexpr auto BufferSize = StaticBufferSize;

    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger const&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    virtual ~Logger() = default;

    /**
     * @brief Constructs a new Logger object with the specified logging level.
     *
     * @param category Logger category name. Can be used in logger messages.
     * @param level Logging level.
     */
    explicit Logger(StringViewType category, Level level = Level::Info)
        : m_category(category) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
        , m_sinks(this)
    {
    }

    /**
     * @brief Constructs a new child Logger object.
     *
     * @param category Logger category name. Can be used in logger messages.
     * @param level Logging level.
     * @param parent Parent logger to inherit sinks from.
     */
    explicit Logger(StringViewType category, Level level, const std::shared_ptr<Logger>& parent)
        : m_parent(parent)
        , m_category(category)
        , m_level(level)
        , m_sinks(this, &parent->m_sinks)
    {
    }

    /**
     * @brief Constructs a new child Logger object.
     *
     * @param category Logger category name. Can be used in logger messages.
     * @param parent Parent logger to inherit sinks and logging level from.
     */
    explicit Logger(StringViewType category, const std::shared_ptr<Logger>& parent)
        : m_parent(parent)
        , m_category(category)
        , m_level(parent->level())
        , m_sinks(this, &parent->m_sinks)
    {
    }

    /**
     * @brief Gets the logger category.
     *
     * @return Logger category.
     */
    [[nodiscard]] auto category() const -> StringViewType
    {
        return StringViewType{m_category};
    }

    /**
     * @brief Adds an existing sink to this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually inserted.
     * @return \b false if the sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        return m_sinks.add_sink(sink);
    }

    /**
     * @brief Creates and emplaces a new sink for this logger.
     *
     * @tparam T Sink type (e.g., ConsoleSink).
     * @tparam Args Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by the specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<template<typename> class T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        return m_sinks.template add_sink<T<Logger>>(std::forward<Args>(args)...);
    }

    /**
     * @brief Removes a sink from this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually removed.
     * @return \b false if the sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        return m_sinks.remove_sink(sink);
    }

    /**
     * @brief Enables or disables a sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if the sink exists and is enabled.
     * @return \b false if the sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool
    {
        return m_sinks.set_sink_enabled(sink, enabled);
    }

    /**
     * @brief Checks if a sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink is enabled.
     * @return \b false if the sink is disabled.
     */
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) const -> bool
    {
        return m_sinks.sink_enabled(sink);
    }

    /**
     * @brief Sets the logging level.
     *
     * @param level Level to be set for this logger (e.g., Log::Level::Info).
     */
    auto set_level(Level level) -> void
    {
        m_level = level;
    }

    /**
     * @brief Gets the logging level.
     *
     * @return Logging level for this logger.
     */
    [[nodiscard]] auto level() const -> Level
    {
        return static_cast<Level>(m_level);
    }

    /**
     * @brief Checks if a particular logging level is enabled for the logger.
     *
     * @param level Log level to check.
     * @return \b true if the specified \p level is enabled.
     * @return \b false if the specified \p level is disabled.
     */
    [[nodiscard]] auto level_enabled(Level level) const noexcept -> bool
    {
        return static_cast<Level>(m_level) >= level;
    }

    /**
     * @brief Emits a new callback-based log message if it fits the specified logging level.
     *
     * Method to emit a log message from a callback return value convertible to the logger string
     * type. Used to postpone formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost if it does not fit the current logging level.
     *
     * @tparam T Invocable type for the callback. Deduced from the argument.
     * @tparam Args Format argument types. Deduced from arguments.
     * @param level Logging level.
     * @param callback Log callback.
     * @param location Caller location (file, line, function).
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename T, typename... Args>
    auto message(Level level, T&& callback, Location location = Location::current(), Args&&... args)
        const -> void
    {
        m_sinks.message(
            level, std::forward<T>(callback), category(), location, std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a new formatted log message if it fits the specified logging level.
     *
     * Method to emit compile-time formatted messages with basic format argument checks.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param level Logging level.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    void
    message(Level level, Format<CharType, std::type_identity_t<Args>...> fmt, Args&&... args) const
    {
        auto callback = [&fmt = fmt.fmt()](auto& buffer, Args&&... args) {
            buffer.format(fmt, std::forward<Args>(args)...);
        };

        this->message(level, std::move(callback), fmt.loc(), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits an informational message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto info(Format<CharType, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Info, std::move(fmt), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a basic informational message.
     *
     * Basic method to emit any message that is convertible to the logger string type
     * or is a callback returning such a string.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from the argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    auto info(T&& message, Location location = Location::current()) const -> void
    {
        this->message(Level::Info, std::forward<T>(message), location);
    }

private:
    std::shared_ptr<Logger> m_parent;
    std::basic_string<Char> m_category;
    LevelDriver<ThreadingPolicy> m_level;
    SinkDriver<Logger, ThreadingPolicy> m_sinks;
};

/**
 * @brief Deduction guide for a constructor call with a string.
 *
 * @tparam String String type for log messages.
 */
template<typename String>
Logger(String, Level = Level::Info) -> Logger<String>;

/**
 * @brief Deduction guide for a constructor call with a char array.
 *
 * @tparam String String type for log messages.
 */
template<typename Char, std::size_t N>
Logger(
    const Char (&)[N], // NOLINT(*-avoid-c-arrays)
    Level = Level::Info) -> Logger<std::basic_string_view<Char>>;

} // namespace PlainCloud::Log
