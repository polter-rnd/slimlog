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

// GCC 11+
// CLang 15+
// libfmt 9.1.0+

namespace PlainCloud::Log {

template<typename String, typename ThreadingPolicy = MultiThreadedPolicy<>>
class Logger {
public:
    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger const&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    ~Logger() = default;

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

    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger<String>>& parent, const Level level)
        : m_parent(parent)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
    {
    }

    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger<String>>& parent)
        : m_parent(parent)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(parent->level())
    {
    }

    auto add_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
    {
        return m_sinks.add_sink(sink);
    }

    template<template<typename> class T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<String>>
    {
        return m_sinks.template add_sink<T<String>>(std::forward<Args>(args)...);
    }

    auto remove_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
    {
        return m_sinks.remove_sink(sink);
    }

    auto set_sink_enabled(const std::shared_ptr<Sink<String>>& sink, bool enabled) -> bool
    {
        return m_sinks.set_sink_enabled(sink, enabled);
    }

    auto sink_enabled(const std::shared_ptr<Sink<String>>& sink) -> bool
    {
        return m_sinks.sink_enabled(sink);
    }

    auto set_level(Level level) -> void
    {
        m_level = level;
    }

    auto level() -> Level
    {
        return static_cast<Level>(m_level);
    }

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
     * @brief Emits and logs new event if the Logger is enabled for specified logging level.
     *
     * @param level Logging level
     * @param location Information about the caller (file, line, function)
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Event message. Use variadic args for `fmt::format`-based formatting.
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

    template<typename... Args>
    inline void emit(Level level, const Format<Args...>& fmt, Args&&... args) const
    {
        auto callback
            = [&fmt](Args&&... args) { return format(fmt.fmt(), std::forward<Args>(args)...); };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void emit(Level level, const WideFormat<Args...>& fmt, Args&&... args) const
    {
        auto callback
            = [&fmt](Args&&... args) { return format(fmt.fmt(), std::forward<Args>(args)...); };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    // Informational

    template<typename... Args>
    inline auto info(const Format<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline auto info(const WideFormat<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename T>
    inline auto info(const T& fmt, const Location& caller = Location::current()) const -> void
    {
        emit(Level::Info, fmt, caller);
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
