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
#include "util.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace PlainCloud::Log {

/** Default buffer size is equal to typical memory page size */
static constexpr size_t DefaultBufferSize = 4096;

namespace Detail {

template<typename T>
struct UnderlyingChar {
    static_assert(Util::AlwaysFalse<T>{}, "Unable to deduce the underlying char type");
};

template<typename T>
    requires std::is_integral_v<typename std::remove_cvref_t<T>::value_type>
struct UnderlyingChar<T> {
    using Type = typename std::remove_cvref_t<T>::value_type;
};

template<typename T>
    requires std::is_array_v<std::remove_cvref_t<T>>
struct UnderlyingChar<T> {
    using Type = typename std::remove_cvref_t<typename std::remove_all_extents_t<T>>;
};

template<typename T>
    requires std::is_integral_v<typename std::remove_pointer_t<T>>
struct UnderlyingChar<T> {
    using Type = typename std::remove_cvref_t<typename std::remove_pointer_t<T>>;
};

template<typename T>
using UnderlyingCharType = typename UnderlyingChar<T>::Type;

} // namespace Detail

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
    typename Char = Detail::UnderlyingCharType<String>,
    typename ThreadingPolicy = MultiThreadedPolicy<>,
    size_t StaticBufferSize = DefaultBufferSize>
class Logger {
public:
    using StringType = String;
    using CharType = Char;

    static constexpr auto BufferSize = StaticBufferSize;

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
    explicit Logger(T&& name, const Level level = Level::Info)
        : m_parent(nullptr)
        , m_category(std::forward<T>(name)) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
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
    explicit Logger(
        T&& name, const std::shared_ptr<Logger>& parent, const Level level = Level::Info)
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
    [[nodiscard]] auto category() const -> String
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
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) const -> bool
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
    auto level() const -> Level
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
        T&& callback,
        Location location = Location::current(),
        Args&&... args) const -> void
    {
        m_sinks.message(
            *this, level, std::forward<T>(callback), location, std::forward<Args>(args)...);
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
    inline void
    message(Level level, Format<CharType, std::type_identity_t<Args>...> fmt, Args&&... args) const
    {
        auto callback = [&fmt = fmt.fmt()](auto& buffer, Args&&... args) {
            buffer.format(fmt, std::forward<Args>(args)...);
        };

        this->message(level, std::move(callback), fmt.loc(), std::forward<Args>(args)...);
    }

    /**
     * @brief Emit informational message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    inline auto
    info(Format<CharType, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Info, std::move(fmt), std::forward<Args>(args)...);
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
    inline auto info(T&& message, Location caller = Location::current()) const -> void
    {
        this->message(Level::Info, std::forward<T>(message), caller);
    }

private:
    const std::shared_ptr<Logger> m_parent;
    const String m_category;
    LevelDriver<ThreadingPolicy> m_level;
    SinkDriver<Logger, ThreadingPolicy> m_sinks;

    template<typename Logger, typename Policy>
    friend class SinkDriver;
};

template<typename String>
Logger(String, Level = Level::Info) -> Logger<String>;

template<typename Char, size_t N>
Logger(
    const Char (&)[N], // NOLINT(*-avoid-c-arrays)
    Level = Level::Info) -> Logger<std::basic_string_view<Char>>;

} // namespace PlainCloud::Log
