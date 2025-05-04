/**
 * @file logger.h
 * @brief Contains the definition of the Logger class.
 */

#pragma once

#include "slimlog/format.h"
#include "slimlog/level.h" // IWYU pragma: export
#include "slimlog/location.h"
#include "slimlog/sink.h"
#include "slimlog/util/os.h"
#include "slimlog/util/types.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace SlimLog {

/**
 * @brief Logger front-end class.
 *
 * The Logger class performs log message filtering and emits messages through specified sinks.
 *
 * @tparam String Type used for logging messages (e.g., `std::string`).
 * @tparam Char Underlying character type for the string.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename String,
    typename Char = Util::Types::UnderlyingCharType<String>,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
class Logger {
public:
    /** @brief String view type for log categories. */
    using StringViewType = std::basic_string_view<Char>;
    /** @brief Base sink type for the logger. */
    using SinkType = Sink<String, Char>;
    /** @brief Time function type for getting the current time. */
    using TimeFunctionType = std::function<std::pair<std::chrono::sys_seconds, std::size_t>()>;

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    virtual ~Logger() = default;

    /**
     * @brief Constructs a new Logger object with the specified logging level.
     *
     * @param category Logger category name.
     * @param level Logging level.
     */
    explicit Logger(
        StringViewType category,
        Level level = Level::Info,
        TimeFunctionType time_func = Util::OS::local_time<std::chrono::system_clock>)
        : m_category(category) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
        , m_level(level)
        , m_time_func(std::move(time_func))
        , m_sinks(this, nullptr)
    {
    }

    explicit Logger(
        Level level, TimeFunctionType time_func = Util::OS::local_time<std::chrono::system_clock>)
        : Logger(StringViewType{DefaultCategory.data()}, level, std::move(time_func))
    {
    }

    explicit Logger(TimeFunctionType time_func = Util::OS::local_time<std::chrono::system_clock>)
        : Logger(StringViewType{DefaultCategory.data()}, Level::Info, std::move(time_func))
    {
    }

    /**
     * @brief Constructs a new child Logger object.
     *
     * @param category Logger category name. Can be used in logger messages.
     * @param level Logging level.
     * @param parent Parent logger to inherit sinks from.
     */
    explicit Logger(StringViewType category, Level level, Logger& parent)
        : m_category(category)
        , m_level(level)
        , m_sinks(this, &parent.m_sinks)
        , m_time_func(parent.m_time_func)
    {
    }

    /**
     * @brief Constructs a new child Logger object.
     *
     * @param category Logger category name. Can be used in logger messages.
     * @param parent Parent logger to inherit sinks and logging level from.
     */
    explicit Logger(StringViewType category, Logger& parent)
        : m_category(category)
        , m_level(parent.level())
        , m_sinks(this, &parent.m_sinks)
        , m_time_func(parent.m_time_func)
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
     * @brief Gets the current time.
     *
     * @return Current time as a pair of local time and nanoseconds.
     * @note The local time is represented as a `std::chrono::sys_seconds` object.
     */
    [[nodiscard]] auto time() const -> std::pair<std::chrono::sys_seconds, std::size_t>
    {
        return m_time_func ? m_time_func()
                           : std::make_pair(std::chrono::sys_seconds{}, std::size_t{});
    }

    /**
     * @brief Adds an existing sink to this logger.
     *
     * @param sink Shared pointer to the sink.
     * @return true if the sink was added, false if it already exists.
     */
    auto add_sink(const std::shared_ptr<SinkType>& sink) -> bool
    {
        return m_sinks.add_sink(sink);
    }

    /**
     * @brief Creates and adds a new formattable sink to this logger.
     *
     * @tparam T Sink type template (e.g., OStreamSink).
     * @tparam SinkBufferSize Size of the internal buffer for the sink.
     * @tparam SinkAllocator Allocator type for the sink buffer.
     * @tparam Args Argument types for the sink constructor.
     * @param args Arguments forwarded to the sink constructor.
     * @return Shared pointer to the created sink.
     */
    template<
        template<typename, typename, std::size_t, typename> // For clang-format < 19
        typename T,
        std::size_t SinkBufferSize = DefaultSinkBufferSize,
        typename SinkAllocator = Allocator,
        typename... Args>
        requires(IsFormattableSink<T<String, Char, SinkBufferSize, SinkAllocator>>)
    auto add_sink(Args&&... args) -> std::shared_ptr<SinkType>
    {
        return m_sinks.template add_sink<T<String, Char, SinkBufferSize, SinkAllocator>>(
            std::forward<Args>(args)...);
    }

    /**
     * @brief Creates and adds a new sink to this logger.
     *
     * @tparam T Sink type template (e.g., OStreamSink).
     * @tparam Args Argument types for the sink constructor.
     * @param args Arguments forwarded to the sink constructor.
     * @return Shared pointer to the created sink.
     */
    template<template<typename, typename> class T, typename... Args>
        requires(!IsFormattableSink<T<String, Char>>)
    auto add_sink(Args&&... args) -> std::shared_ptr<SinkType>
    {

        return m_sinks.template add_sink<T<String, Char>>(std::forward<Args>(args)...);
    }

    /**
     * @brief Removes a sink from this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually removed.
     * @return \b false if the sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<SinkType>& sink) -> bool
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
    auto set_sink_enabled(const std::shared_ptr<SinkType>& sink, bool enabled) -> bool
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
    [[nodiscard]] auto sink_enabled(const std::shared_ptr<SinkType>& sink) const -> bool
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
    void message(Level level, Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const
    {
        auto callback = [&fmt = fmt.fmt()](auto& buffer, Args&&... args) {
            buffer.format(fmt, std::forward<Args>(args)...);
        };

        this->message(level, std::move(callback), fmt.loc(), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a trace message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto trace(Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Trace, std::move(fmt), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a basic trace message.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from the argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    auto trace(T&& message, Location location = Location::current()) const -> void
    {
        this->message(Level::Trace, std::forward<T>(message), location);
    }

    /**
     * @brief Emits a debug message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto debug(Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Debug, std::move(fmt), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a basic debug message.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from the argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    auto debug(T&& message, Location location = Location::current()) const -> void
    {
        this->message(Level::Debug, std::forward<T>(message), location);
    }

    /**
     * @brief Emits a warning message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto warning(Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Warning, std::move(fmt), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits an informational message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto info(Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
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

    /**
     * @brief Emits a basic warning message.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from the argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    auto warning(T&& message, Location location = Location::current()) const -> void
    {
        this->message(Level::Warning, std::forward<T>(message), location);
    }

    /**
     * @brief Emits an error message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto error(Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Error, std::move(fmt), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a basic error message.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from the argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    auto error(T&& message, Location location = Location::current()) const -> void
    {
        this->message(Level::Error, std::forward<T>(message), location);
    }

    /**
     * @brief Emits a fatal message.
     *
     * @tparam Args Format argument types. Deduced from arguments.
     * @param fmt Format string. See `fmt::format` documentation for details.
     * @param args Format arguments. Use variadic args for `fmt::format`-based formatting.
     */
    template<typename... Args>
    auto fatal(Format<Char, std::type_identity_t<Args>...> fmt, Args&&... args) const -> void
    {
        this->message(Level::Fatal, std::move(fmt), std::forward<Args>(args)...);
    }

    /**
     * @brief Emits a basic fatal message.
     *
     * @tparam T Message type. Can be either a string or a callback. Deduced from the argument.
     *
     * @param message Log message or callback.
     * @param location Caller location (file, line, function).
     */
    template<typename T>
    auto fatal(T&& message, Location location = Location::current()) const -> void
    {
        this->message(Level::Fatal, std::forward<T>(message), location);
    }

private:
    std::basic_string<Char> m_category;
    LevelDriver<ThreadingPolicy> m_level;
    TimeFunctionType m_time_func;
    SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator> m_sinks;
    static constexpr std::array<Char, 8> DefaultCategory{'d', 'e', 'f', 'a', 'u', 'l', 't', '\0'};
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

} // namespace SlimLog
