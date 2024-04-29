#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <version>

// GCC 11+
// CLang 15+
// libfmt 9.1.0+

namespace PlainCloud::Log {
// Forward declaration of namespace to make IWYU happy
}

#include "format.h"
#include "location.h"

namespace PlainCloud::Log {

enum class Level { Fatal, Error, Warning, Info, Debug, Trace };

template<typename StringT>
class Logger {
public:
    class Sink {
    public:
        Sink() = default;
        Sink(Sink const&) = default;
        Sink(Sink&&) noexcept = default;
        auto operator=(Sink const&) -> Sink& = default;
        auto operator=(Sink&&) noexcept -> Sink& = default;
        virtual ~Sink() = default;

        virtual auto
        emit(Log::Level level, const StringT& message, const Log::Location& location) const -> void
            = 0;
        virtual auto flush() -> void = 0;
    };

    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger const&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    ~Logger() = default;

    template<typename T>
    explicit Logger(
        T&& name,
        const Log::Level level = Log::Level::Info,
        const std::initializer_list<std::shared_ptr<Sink>>& sinks = {})
        : m_parent(nullptr)
        , m_name(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
    {
        m_sinks.reserve(sinks.size());
        for (const auto& sink : sinks) {
            m_sinks.emplace(sink, true);
        }
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

    auto add_sink(const std::shared_ptr<Sink>& sink) -> bool
    {
        return m_sinks.insert_or_assign(sink, true).second;
    }

    template<template<typename> class T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink>
    {
        return m_sinks
            .insert_or_assign(std::make_shared<T<StringT>>(std::forward<Args>(args)...), true)
            .first->first;
    }

    auto remove_sink(const std::shared_ptr<Sink>& sink) -> bool
    {
        return m_sinks.erase(sink) == 1;
    }

    auto set_sink_enabled(const std::shared_ptr<Sink>& sink, bool enabled) -> bool
    {
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            itr->second = enabled;
            return true;
        }
        return false;
    }

    auto sink_enabled(const std::shared_ptr<Sink>& sink) -> bool
    {
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            return itr->second;
        }
        return false;
    }

    auto set_level(Log::Level level) -> void
    {
        m_level = level;
    }

    auto level() -> Log::Level
    {
        return m_level;
    }

    template<typename T, typename... Args>
        requires std::invocable<T, Args...>
    inline auto emit(
        const Log::Level level,
        const T& callback,
        const Log::Location& location = Log::Location::current(),
        Args&&... args) const -> void
    {
        if (m_level < level) {
            return;
        }

        auto sinks{m_sinks};
        for (auto parent{m_parent}; parent; parent = parent->m_parent) {
            sinks.insert(parent->m_sinks.cbegin(), parent->m_sinks.cend());
        }
        for (const auto& sink : sinks) {
            if (sink.second) {
                // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                sink.first->emit(level, callback(std::forward<Args>(args)...), location);
            }
        }
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
    std::unordered_map<std::shared_ptr<Sink>, bool> m_sinks;
    std::shared_ptr<Logger<StringT>> m_parent;
    StringT m_name;
    Log::Level m_level;
};

template<typename T>
Logger(
    T&&,
    Log::Level = Log::Level::Info,
    const std::initializer_list<std::shared_ptr<typename Logger<T>::Sink>>& sinks = {})
    -> Logger<T>;

template<typename T, size_t N>
Logger(
    const T (&)[N], // NOLINT(*-avoid-c-arrays)
    Log::Level = Log::Level::Info,
    const std::initializer_list<std::shared_ptr<typename Logger<T>::Sink>>& sinks = {})
    -> Logger<std::basic_string_view<T>>;

} // namespace PlainCloud::Log
