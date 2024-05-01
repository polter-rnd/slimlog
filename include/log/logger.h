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

template<typename StringT, typename ThreadingPolicy = MultiThreadedPolicy<>>
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
        const Log::Level level = Log::Level::Info,
        const std::initializer_list<std::shared_ptr<Sink<StringT>>>& sinks = {})
        : m_parent(nullptr)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
        , m_sinks(sinks)
    {
    }

    template<typename T>
    explicit Logger(
        T&& name, const std::shared_ptr<Logger<StringT>>& parent, const Log::Level level)
        : m_parent(parent)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
    {
    }

    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger<StringT>>& parent)
        : m_parent(parent)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(parent->level())
    {
    }

    auto add_sink(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        return m_sinks.add_sink(sink);
    }

    template<template<typename> class T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<StringT>>
    {
        return m_sinks.template add_sink<T<StringT>>(std::forward<Args>(args)...);
    }

    auto remove_sink(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        return m_sinks.remove_sink(sink);
    }

    auto set_sink_enabled(const std::shared_ptr<Sink<StringT>>& sink, bool enabled) -> bool
    {
        return m_sinks.set_sink_enabled(sink, enabled);
    }

    auto sink_enabled(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        return m_sinks.sink_enabled(sink);
    }

    auto set_level(Log::Level level) -> void
    {
        m_level = level;
    }

    auto level() -> Log::Level
    {
        return static_cast<Level>(m_level);
    }

    template<typename T, typename... Args>
        requires std::invocable<T, Args...>
    inline auto emit(
        const Log::Level level,
        const T& callback,
        const Log::Location& location = Log::Location::current(),
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
    inline auto emit(
        const Log::Level level,
        const T& message,
        const Log::Location& location = Log::Location::current()) const -> void
    {
        auto callback = [](const T& message) -> const T& { return message; };
        emit(level, callback, location, message);
    }

    template<typename... Args>
    inline void emit(Log::Level level, const Log::Format<Args...>& fmt, Args&&... args) const
    {
        auto callback = [&fmt](Args&&... args) {
            return Log::format(fmt.fmt(), std::forward<Args>(args)...);
        };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline void emit(Log::Level level, const Log::WideFormat<Args...>& fmt, Args&&... args) const
    {
        auto callback = [&fmt](Args&&... args) {
            return Log::format(fmt.fmt(), std::forward<Args>(args)...);
        };
        emit(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    // Informational

    template<typename... Args>
    inline auto info(const Log::Format<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Log::Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline auto info(const Log::WideFormat<Args...>& fmt, Args&&... args) const -> void
    {
        emit(Log::Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename T>
    inline auto info(const T& fmt, const Log::Location& caller = Log::Location::current()) const
        -> void
    {
        emit(Log::Level::Info, fmt, caller);
    }

private:
    std::shared_ptr<Logger<StringT>> m_parent;
    StringT m_name;
    LevelDriver<ThreadingPolicy> m_level;
    SinkDriver<StringT, ThreadingPolicy> m_sinks;

    template<typename T, typename Policy>
    friend class SinkDriver;
};

template<typename T, typename SinkT = Sink<T>>
Logger(
    T, Level = Level::Info, std::initializer_list<std::shared_ptr<SinkT>> = std::initializer_list{})
    -> Logger<T>;

template<typename T, size_t N, typename SinkT = Sink<T>>
Logger(
    const T (&)[N], // NOLINT(*-avoid-c-arrays)
    Level = Level::Info,
    std::initializer_list<std::shared_ptr<SinkT>> = std::initializer_list{})
    -> Logger<std::basic_string_view<T>>;

} // namespace PlainCloud::Log
