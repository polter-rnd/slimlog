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
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace PlainCloud::Log {

namespace {

template<typename>
struct AlwaysFalse : std::false_type { };

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
 * log.add_sink<Log::OStreamSink>(std::cerr);
 * log.info("Hello {}!", "World");
 * ```
 *
 * @tparam String String type for logging messages (e.g. `std::string`).
 *                Can be deduced from logger name.
 * @tparam ThreadingPolicy Threading policy used for operating over sinks and log level
 *                         (e.g. SingleThreadedPolicy or MultiThreadedPolicy).
 */
template<
    typename String,
    typename Char = UnderlyingType<String>,
    typename ThreadingPolicy = MultiThreadedPolicy<>>
class Logger {
public:
    using StringType = String;
    using CharType = Char;
    using StreamBuffer = std::basic_stringstream<Char>;

    Logger(Logger const&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(Logger const&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    virtual ~Logger() = default;

    /**
     * @brief Construct a new %Logger object with specified logging level.
     *
     * @tparam T String type for logger category name. Can be deduced from argument.
     * @param category %Logger category name. Can be used in logger messages.
     * @param level Logging level.
     * @param sinks Sinks to be added upon creation.
     */
    template<typename T>
    explicit Logger(
        T&& name,
        const Level level = Level::Info,
        const std::initializer_list<std::shared_ptr<Sink<Logger>>>& sinks = {})
        : m_parent(nullptr)
        , m_category(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
        , m_sinks(sinks)
    {
    }

    /**
     * @brief Construct a new child %Logger object.
     *
     * @tparam T String type for logger category name. Can be deduced from argument.
     * @param name %Logger category name. Can be used in logger messages.
     * @param parent Parent logger to inherit sinks from.
     * @param level Logging level.
     */
    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger>& parent, const Level level)
        : m_parent(parent)
        , m_category(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
    {
    }

    /**
     * @brief Construct a new child %Logger object.
     *
     * @tparam T String type for logger category name. Can be deduced from argument.
     * @param name %Logger category name. Can be used in logger messages.
     * @param parent Parent logger to inherit sinks and logging level from.
     */
    template<typename T>
    explicit Logger(T&& name, const std::shared_ptr<Logger>& parent)
        : m_parent(parent)
        , m_category(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(parent->level())
    {
    }

    /**
     * @brief Get logger name
     *
     * @return %Logger name
     */
    auto category() const noexcept -> const String&
    {
        return m_category;
    }

    /**
     * @brief Add existing sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
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
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        return m_sinks.template add_sink<T<Logger>>(std::forward<Args>(args)...);
    }

    /**
     * @brief Remove sink from this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually removed.
     * @return \b false if sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
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
    auto set_sink_enabled(const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool
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
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) -> bool
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
    inline auto message(
        const Level level,
        const T& callback,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        m_sinks.message(*this, level, callback, location, std::forward<Args>(args)...);
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
        requires std::same_as<CharType, char>
    inline void message(Level level, const Format<Args...>& fmt, Args&&... args) const
    {
        auto callback = [&fmt](auto& buffer, Args&&... args) {
            format_to(std::ostreambuf_iterator(buffer), fmt.fmt(), std::forward<Args>(args)...);
        };

        this->message(level, callback, fmt.loc(), std::forward<Args>(args)...);
    }

    /**
     * @brief Emit new formatted wide string log message if it fits for specified logging level.
     *
     * Method to emit compile-time wide string formatted messages wich basic format argument
     * checks.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param level Logging level.
     * @param fmt Format wide string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
        requires std::same_as<CharType, wchar_t>
    inline void message(Level level, const WideFormat<Args...>& fmt, Args&&... args) const
    {
        auto callback = [&fmt](auto& buffer, Args&&... args) {
#ifdef __cpp_lib_format
            format_to(std::ostreambuf_iterator(buffer), fmt.fmt(), std::forward<Args>(args)...);
#else
            format_to(
                std::ostreambuf_iterator(buffer), fmt.fmt().get(), std::forward<Args>(args)...);
#endif
        };

        this->message(level, callback, fmt.loc(), std::forward<Args>(args)...);
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
        this->message(Level::Info, fmt, std::forward<Args>(args)...);
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
        this->message(Level::Info, fmt, std::forward<Args>(args)...);
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
        this->message(Level::Info, message, caller);
    }

private:
    std::shared_ptr<Logger> m_parent;
    String m_category;
    LevelDriver<ThreadingPolicy> m_level;
    SinkDriver<Logger, ThreadingPolicy> m_sinks;

    template<typename Logger, typename Policy>
    friend class SinkDriver;
};

template<
    typename String,
    typename Char,
    typename Sinks = std::initializer_list<std::shared_ptr<Sink<Logger<String, Char>>>>>
Logger(String, Char, Level = Level::Info, Sinks = Sinks{}) -> Logger<String, Char>;

template<
    typename String,
    typename Char = String,
    typename Sinks = std::initializer_list<std::shared_ptr<Sink<Logger<String, Char>>>>,
    size_t N>
Logger(
    const String (&)[N], // NOLINT(*-avoid-c-arrays)
    Level = Level::Info,
    Sinks = Sinks{}) -> Logger<std::basic_string_view<String>, Char>;

} // namespace PlainCloud::Log
