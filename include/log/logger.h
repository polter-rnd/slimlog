/**
 * @file logger.h
 * @brief Contains definition of Logger class.
 */

#pragma once

#include "format.h"
#include "level.h"
#include "location.h"
#include "policy.h"
#include "sink.h"

#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <utility>

namespace PlainCloud::Log {

/**
 * @brief A logger front-end class.
 *
 * Permorms log message filtering and emitting through specified sinks.
 * Sink object represents a back-end used for emitting the message.
 *
 * Usage example:
 *
 * ```cpp
 * Log::Logger log("main");
 * log.add_sink<Log::ConsoleSink>();
 * log.info("Hello {}!", "World");
 * ```
 *
 * @tparam String String type for logging messages (e.g. `std::string`).
 *                Can be deduced from logger name.
 * @tparam ThreadingPolicy Threading policy used for operating over sinks and log level
 *                         (e.g. SingleThreadedPolicy or MultiThreadedPolicy).
 */
template<typename String, typename ThreadingPolicy = MultiThreadedPolicy<>>
class Logger {
public:
    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger const&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    ~Logger() = default;

    /**
     * @brief Construct a new %Logger object with specified logging level.
     *
     * @tparam T String type for logger name. Can be deduced from argument.
     * @param name %Logger name. Can be used in logger messages.
     * @param level Logging level.
     * @param sinks Sinks to be added upon creation.
     */
    template<typename T>
    explicit Logger(
        T&& name,
        const Level level = Level::Info,
        const std::initializer_list<std::shared_ptr<Sink<String>>>& sinks = {})
        : m_parent(nullptr)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
        , m_sinks(sinks)
    {
    }

    /**
     * @brief Construct a new child %Logger object.
     *
     * @tparam T String type for logger name. Can be deduced from argument.
     * @param name %Logger name. Can be used in logger messages.
     * @param parent Parent logger to inherit sinks from.
     * @param level Logging level.
     */
    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger<String>>& parent, const Level level)
        : m_parent(parent)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
    {
    }

    /**
     * @brief Construct a new child %Logger object.
     *
     * @tparam T String type for logger name. Can be deduced from argument.
     * @param name %Logger name. Can be used in logger messages.
     * @param parent Parent logger to inherit sinks and logging level from.
     */
    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger<String>>& parent)
        : m_parent(parent)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(parent->level())
    {
    }

    /**
     * @brief Add existing sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
    {
        return m_sinks.add_sink(sink);
    }

    /**
     * @brief Create and emplace a new sink for this logger.
     *
     * @tparam T %Sink type (e.g. ConsoleSink)
     * @tparam Args %Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<template<typename> class T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<String>>
    {
        return m_sinks.template add_sink<T<String>>(std::forward<Args>(args)...);
    }

    /**
     * @brief Remove sink from this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually removed.
     * @return \b false if sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
    {
        return m_sinks.remove_sink(sink);
    }

    /**
     * @brief Enable or disable sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if sink exists and enabled.
     * @return \b false if sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<Sink<String>>& sink, bool enabled) -> bool
    {
        return m_sinks.set_sink_enabled(sink, enabled);
    }

    /**
     * @brief Check if sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink is enabled.
     * @return \b false if sink is disabled.
     */
    auto sink_enabled(const std::shared_ptr<Sink<String>>& sink) -> bool
    {
        return m_sinks.sink_enabled(sink);
    }

    /**
     * @brief Set logging level.
     *
     * @param level Level to be set for this logger (e.g. Log::Level::Info).
     */
    auto set_level(Level level) -> void
    {
        m_level = level;
    }

    /**
     * @brief Get logging level.
     *
     * @return Logging level for this logger.
     */
    auto level() -> Level
    {
        return static_cast<Level>(m_level);
    }

    /**
     * @brief Emit new callback-based log message if it fits for specified logging level.
     *
     * Method to emit log message from callback return value convertible to logger string type.
     * Used to postpone formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost in case if it does not fit for current logging level.
     *
     * @tparam T Invocable type for the callback. Deduced from argument.
     * @tparam Args Format argument types. Deduced from arguments.
     * @param level Logging level.
     * @param callback Log callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T, typename... Args>
        requires std::invocable<T, Args...>
    inline auto emit(
        const Level level,
        const T& callback,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        m_sinks.emit(*this, level, callback, location, std::forward<Args>(args)...);
    }

    /**
     * @brief Emit new log message if it fits for specified logging level.
     *
     * Basic method to emit any message that is convertible to logger string type.
     *
     * @tparam T String type for the message. Deduced from argument.
     * @param level Logging level.
     * @param message Log message.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
        requires(!std::invocable<T>)
    inline auto
    emit(const Level level, const T& message, const Location& location = Location::current()) const
        -> void
    {
        auto callback = [](const T& message) -> const T& { return message; };
        emit(level, callback, location, message);
    }

    /**
     * @brief Emit new formatted log message if it fits for specified logging level.
     *
     * Method to emit compile-time formatted messages wich basic format argument checks.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param level Logging level.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    inline void emit(Level level, const Format<Args...>& fmt, Args&&... args) const
    {
        auto callback
            = [&fmt](Args&&... args) { return format(fmt.fmt(), std::forward<Args>(args)...); };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    /**
     * @brief Emit new formatted wide string log message if it fits for specified logging level.
     *
     * Method to emit compile-time wide string formatted messages wich basic format argument checks.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param level Logging level.
     * @param fmt Format wide string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    inline void emit(Level level, const WideFormat<Args...>& fmt, Args&&... args) const
    {
        auto callback
            = [&fmt](Args&&... args) { return format(fmt.fmt(), std::forward<Args>(args)...); };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    /**
     * @brief Emit informational message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    inline auto info(const Format<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Level::Info, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Emit informational wide string message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format wide string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    inline auto info(const WideFormat<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Level::Info, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Emit basic informational message.
     *
     * Basic method to emit any message that is convertible to logger string type
     * or is a callback returning such a string.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    inline auto info(const T& message, const Location& caller = Location::current()) const -> void
    {
        emit(Level::Info, message, caller);
    }

private:
    std::shared_ptr<Logger<String>> m_parent;
    String m_name;
    LevelDriver<ThreadingPolicy> m_level;
    SinkDriver<String, ThreadingPolicy> m_sinks;

    template<typename T, typename Policy>
    friend class SinkDriver;
};

template<typename T, typename SinksT = std::initializer_list<std::shared_ptr<Sink<T>>>>
Logger(T, Level = Level::Info, SinksT = SinksT{}) -> Logger<T>;

template<
    typename T,
    typename SinksT = std::initializer_list<std::shared_ptr<Sink<T>>>,
    size_t N>
Logger(
    const T (&)[N], // NOLINT(*-avoid-c-arrays)
    Level = Level::Info,
    SinksT = SinksT{}) -> Logger<std::basic_string_view<T>>;

} // namespace PlainCloud::Log
