/**
 * @file logger.h
 * @brief Contains the declaration of the Logger class.
 */

#pragma once

#include "slimlog/format.h"
#include "slimlog/location.h" // IWYU pragma: export
#include "slimlog/record.h" // IWYU pragma: export
#include "slimlog/sink.h" // IWYU pragma: export
#include "slimlog/threading.h" // IWYU pragma: export
#include "slimlog/util/os.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace SlimLog {

/**
 * @brief Converts a string to a `std::basic_string_view`.
 *
 * This function takes a String object and returns a `std::basic_string_view`
 * of the same character type.
 *
 * @tparam String String type.
 * @tparam Char Type of the characters in the string.
 * @param str The input string object to be converted.
 * @return Value of type, convertible to `std::basic_string_view<Char>`.
 */
template<typename String, typename Char>
struct ConvertString {
    // auto operator()(const String&) const -> std::basic_string_view<Char> = delete;
    auto operator()(const String&, auto&) const -> std::basic_string_view<Char> = delete;
};

/** @cond */
namespace Detail {

template<typename String, typename Char>
concept HasConvertString
    = requires(String value, FormatBuffer<Char, DefaultBufferSize, std::allocator<Char>> buf) {
          {
              ConvertString<String, Char>{}(value, buf)
          } -> std::convertible_to<std::basic_string_view<Char>>;
      };

} // namespace Detail
/** @endcond */

/**
 * @brief Logger front-end class.
 *
 * The Logger class performs log message filtering and emits messages through specified sinks.
 * Loggers can only be created using the static create() methods and must be managed with
 * shared_ptr.
 *
 * @tparam String Type used for logging messages (e.g., `std::string`).
 * @tparam Char Underlying character type for the string.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename Char = char,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
class Logger
    : public std::enable_shared_from_this<Logger<Char, ThreadingPolicy, BufferSize, Allocator>> {
public:
    /** @brief String view type for log categories. */
    using StringViewType = std::basic_string_view<Char>;
    /** @brief Base sink type for the logger. */
    using SinkType = Sink<Char>;
    /** @brief Time function type for getting the current time. */
    using TimeFunctionType = std::pair<std::chrono::sys_seconds, std::size_t> (*)();

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;
    auto operator=(Logger&&) -> Logger& = delete;

    /**
     * @brief Creates a new Logger object with the specified category and level.
     *
     * @param category Logger category name.
     * @param level Logging level.
     * @return Shared pointer to the created logger.
     */
    static auto create(
        StringViewType category = StringViewType{DefaultCategory.data()},
        Level level = Level::Info) // For clang-format < 19
        -> std::shared_ptr<Logger>
    {
        return std::shared_ptr<Logger>(new Logger(category, level));
    }

    /**
     * @brief Creates a new Logger object with default category.
     *
     * @param level Logging level.
     * @return Shared pointer to the created logger.
     */
    static auto create(Level level) -> std::shared_ptr<Logger>
    {
        return std::shared_ptr<Logger>(new Logger(level));
    }

    /**
     * @brief Creates a new child Logger object.
     *
     * @param parent Parent logger to inherit sinks from.
     * @param category Logger category name. Can be used in logger messages.
     * @param level Logging level.
     * @return Shared pointer to the created logger.
     */
    static auto create(const std::shared_ptr<Logger>& parent, StringViewType category, Level level)
        -> std::shared_ptr<Logger>
    {
        auto logger = std::shared_ptr<Logger>(new Logger(parent, category, level));
        if (parent) {
            parent->add_child(logger);
            logger->update_propagated_sinks();
        }
        return logger;
    }

    /**
     * @brief Creates a new child Logger object with level inherited from parent.
     *
     * @param parent Parent logger to inherit sinks.
     * @param category Logger category name. Can be used in logger messages.
     * @return Shared pointer to the created logger.
     */
    static auto create(const std::shared_ptr<Logger>& parent, StringViewType category)
        -> std::shared_ptr<Logger>
    {
        auto logger = std::shared_ptr<Logger>(new Logger(parent, category));
        if (parent) {
            parent->add_child(logger);
            logger->update_propagated_sinks();
        }
        return logger;
    }

    /**
     * @brief Creates a new child Logger object with category inherited from parent.
     *
     * @param parent Parent logger to inherit sinks.
     * @param level Logging level.
     * @return Shared pointer to the created logger.
     */
    static auto create(const std::shared_ptr<Logger>& parent, Level level)
        -> std::shared_ptr<Logger>
    {
        auto logger = std::shared_ptr<Logger>(new Logger(parent, level));
        if (parent) {
            parent->add_child(logger);
            logger->update_propagated_sinks();
        }
        return logger;
    }

    /**
     * @brief Creates a new child Logger object with category and level inherited from parent.
     *
     * @param parent Parent logger to inherit sinks.
     * @return Shared pointer to the created logger.
     */
    static auto create(const std::shared_ptr<Logger>& parent) -> std::shared_ptr<Logger>
    {
        auto logger = std::shared_ptr<Logger>(new Logger(parent));
        if (parent) {
            parent->add_child(logger);
            logger->update_propagated_sinks();
        }
        return logger;
    }

    /**
     * @brief Destructor for the Logger class.
     */
    virtual ~Logger() = default;

    /**
     * @brief Gets the logger category.
     *
     * @return Logger category.
     */
    [[nodiscard]] auto category() const -> StringViewType;

    /**
     * @brief Adds an existing sink to this logger.
     *
     * @param sink Shared pointer to the sink.
     * @return true if the sink was added, false if it already exists.
     */
    auto add_sink(const std::shared_ptr<SinkType>& sink) -> bool;

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
        template<typename, std::size_t, typename> // For clang-format < 19
        typename T,
        std::size_t SinkBufferSize = DefaultSinkBufferSize,
        typename SinkAllocator = Allocator,
        typename... Args>
        requires(IsFormattableSink<T<Char, SinkBufferSize, SinkAllocator>>)
    auto add_sink(Args&&... args) -> std::shared_ptr<SinkType>
    {
        auto sink
            = std::make_shared<T<Char, SinkBufferSize, SinkAllocator>>(std::forward<Args>(args)...);
        return add_sink(sink) ? sink : nullptr;
    }

    /**
     * @brief Creates and adds a new sink to this logger.
     *
     * @tparam T Sink type template (e.g., OStreamSink).
     * @tparam Args Argument types for the sink constructor.
     * @param args Arguments forwarded to the sink constructor.
     * @return Shared pointer to the created sink.
     */
    template<template<typename> class T, typename... Args>
        requires(!IsFormattableSink<T<Char>>)
    auto add_sink(Args&&... args) -> std::shared_ptr<SinkType>
    {
        auto sink = std::make_shared<T<Char>>(std::forward<Args>(args)...);
        return add_sink(sink) ? sink : nullptr;
    }

    /**
     * @brief Removes a sink from this logger.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually removed.
     * @return \b false if the sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<SinkType>& sink) -> bool;

    /**
     * @brief Enables or disables a sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if the sink exists and is enabled.
     * @return \b false if the sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<SinkType>& sink, bool enabled) -> bool;

    /**
     * @brief Checks if a sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink is enabled.
     * @return \b false if the sink is disabled.
     */
    [[nodiscard]] auto sink_enabled(const std::shared_ptr<SinkType>& sink) const -> bool;

    /**
     * @brief Sets whether log messages should propagate to parent loggers.
     *
     * @param enabled If true, log messages will propagate to parent loggers.
     */
    auto set_propagate(bool enabled) -> void;

    /**
     * @brief Sets the logging level.
     *
     * @param level Level to be set for this logger (e.g., Log::Level::Info).
     */
    auto set_level(Level level) -> void;

    /**
     * @brief Sets the time function used for log timestamps.
     *
     * @param time_func Time function to be set for this logger.
     */
    auto set_time_func(TimeFunctionType time_func) -> void;

    /**
     * @brief Gets the logging level.
     *
     * @return Logging level for this logger.
     */
    [[nodiscard]] auto level() const -> Level;

    /**
     * @brief Checks if a particular logging level is enabled for the logger.
     *
     * @param level Log level to check.
     * @return \b true if the specified \p level is enabled.
     * @return \b false if the specified \p level is disabled.
     */
    [[nodiscard]] auto level_enabled(Level level) const noexcept -> bool;

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
    auto message(
        Level level,
        const T& callback,
        Location location = Location::current(), // cppcheck-suppress passedByValue
        Args&&... args) const -> void
    {
        using FormatBufferType = FormatBuffer<Char, BufferSize, Allocator>;
        using RecordType = Record<Char>;

        if (static_cast<Level>(m_level) < level) [[unlikely]] {
            return;
        }

        FormatBufferType buffer; // NOLINT(misc-const-correctness)
        RecordType record;

        // Flag to check that message has been evaluated
        bool evaluated = false;

        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        // TODO: speed up this. Take first sink and evaluate, then loop through others.
        for (const auto sink : m_propagated_sinks) {
            if (!evaluated) [[unlikely]] {
                evaluated = true;
                record
                    = {static_cast<TimeFunctionType>(m_time_func)(),
                       {},
                       m_category,
                       location.file_name(),
                       location.function_name(),
                       static_cast<std::size_t>(location.line()),
                       Util::OS::thread_id(),
                       level};

                using BufferRefType = std::add_lvalue_reference_t<FormatBufferType>;
                using RecordStringViewType = RecordStringView<Char>;
                if constexpr (std::is_invocable_v<T, BufferRefType, Args...>) {
                    // Callable with buffer argument: message will be stored in buffer.
                    // NOLINTNEXTLINE(*-use-after-move,*-invalid-access-moved)
                    callback(buffer, std::forward<Args>(args)...);
                    record.message = RecordStringViewType{buffer.data(), buffer.size()};
                } else if constexpr (std::is_invocable_v<T, Args...>) {
                    using RetType = typename std::invoke_result_t<T, Args...>;
                    if constexpr (std::is_void_v<RetType>) {
                        // Void callable without arguments: there is no message, just a callback
                        callback(std::forward<Args>(args)...);
                        break;
                    } else {
                        // Non-void callable without arguments: message is the return value
                        auto message = callback(std::forward<Args>(args)...);
                        if constexpr (std::is_convertible_v<RetType, RecordStringViewType>) {
                            record.message = RecordStringViewType{std::move(message)};
                        } else {
                            record.message = message;
                        }
                    }
                } else if constexpr (std::is_convertible_v<T, RecordStringViewType>) {
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    record.message = RecordStringViewType{callback};
                } else if constexpr (std::is_convertible_v<T, StringViewType>) {
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    record.message = RecordStringViewType{StringViewType{callback}};
                } else if constexpr (Detail::HasConvertString<T, Char>) {
                    record.message
                        = RecordStringView<Char>{ConvertString<T, Char>{}(callback, buffer)};
                } else {
                    // Either void callback or string type convertible to logger string type
                    record.message = callback;
                }
            }

            sink->message(record);
        }
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
        using FormatBufferType = FormatBuffer<Char, BufferSize, Allocator>;
        auto callback = [&fmt = fmt.fmt()](FormatBufferType& buffer, Args&&... args) {
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

    /**
     * @brief Returns a pointer to the parent sink (or `nullptr` if none).
     *
     * @return Pointer to the parent sink.
     */
    auto parent() -> std::shared_ptr<Logger>;

    /**
     * @brief Sets the parent sink object.
     *
     * @param parent Pointer to the parent logger.
     */
    auto set_parent(const std::shared_ptr<Logger>& parent) -> void;

protected:
    /**
     * @brief Adds a child logger.
     *
     * @param child Weak pointer to the child logger.
     */
    auto add_child(const std::shared_ptr<Logger>& child) -> void;

    /**
     * @brief Removes a child logger.
     *
     * @param child Pointer to the child logger.
     */
    auto remove_child(const std::shared_ptr<Logger>& child) -> void;

private:
    /**
     * @brief Constructs a new Logger object with the specified category and level.
     *
     * @param category Logger category name.
     * @param level Logging level.
     */
    explicit Logger(
        StringViewType category = StringViewType{DefaultCategory.data()},
        Level level = Level::Info);

    /**
     * @brief Constructs a new Logger object with default category.
     *
     * @param level Logging level.
     */
    explicit Logger(Level level);

    /**
     * @brief Constructs a new child Logger object.
     *
     * @param parent Parent logger to inherit sinks from.
     * @param category Logger category name. Can be used in logger messages.
     * @param level Logging level.
     */
    explicit Logger(const std::shared_ptr<Logger>& parent, StringViewType category, Level level);

    /**
     * @brief Constructs a new child Logger object with level inherited from parent.
     *
     * @param parent Parent logger to inherit sinks.
     * @param category Logger category name. Can be used in logger messages.
     */
    explicit Logger(const std::shared_ptr<Logger>& parent, StringViewType category);

    /**
     * @brief Constructs a new child Logger object with category inherited from parent.
     *
     * @param parent Parent logger to inherit sinks.
     * @param level Logging level.
     */
    explicit Logger(const std::shared_ptr<Logger>& parent, Level level);

    /**
     * @brief Constructs a new child Logger object with category and level inherited from parent.
     *
     * @param parent Parent logger to inherit sinks.
     */
    explicit Logger(const std::shared_ptr<Logger>& parent);

    /**
     * @brief Recursively updates the propagated sinks for
     *        the current logger and its children.
     * @param visited Set of visited loggers to avoid cycles.
     */
    auto update_propagated_sinks(std::unordered_set<Logger*> visited = {}) -> void;

    std::basic_string<Char> m_category;
    AtomicWrapper<Level, ThreadingPolicy> m_level;
    AtomicWrapper<TimeFunctionType, ThreadingPolicy> m_time_func;
    AtomicWrapper<bool, ThreadingPolicy> m_propagate;
    std::shared_ptr<Logger> m_parent;
    std::vector<std::weak_ptr<Logger>> m_children;
    std::vector<SinkType*> m_propagated_sinks;
    std::unordered_map<std::shared_ptr<SinkType>, bool> m_sinks;
    mutable ThreadingPolicy::Mutex m_mutex;
    static constexpr std::array<Char, 8> DefaultCategory{'d', 'e', 'f', 'a', 'u', 'l', 't', '\0'};
};

/**
 * @brief Creates a logger with automatic character type deduction from string view.
 *
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @tparam Char Character type deduced from string view.
 * @param category Logger category name.
 * @param level Logging level.
 * @return Shared pointer to the created logger.
 */
template<
    typename Char,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
auto create_logger(std::basic_string_view<Char> category, Level level = Level::Info)
    -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    using LoggerType = Logger<Char, ThreadingPolicy, BufferSize, Allocator>;
    return LoggerType::create(category, level);
}

/**
 * @brief Creates a logger with automatic character type deduction from string literal.
 *
 * @tparam Char Character type for the string.
 * @tparam N Size of the character array.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @param category Logger category name.
 * @param level Logging level.
 * @return Shared pointer to the created logger.
 */
template<
    typename Char,
    std::size_t N,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
auto create_logger(const Char (&category)[N], Level level = Level::Info)
    -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    return create_logger<Char>(category, level);
}

/**
 * @brief Creates a logger with only a level specified.
 *
 * @tparam Char Character type for the logger.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @param level Logging level.
 * @return Shared pointer to the created logger.
 */
template<
    typename Char = char,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
auto create_logger(Level level = Level::Info)
    -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    using LoggerType = Logger<Char, ThreadingPolicy, BufferSize, Allocator>;
    return LoggerType::create(level);
}

/**
 * @brief Creates a child logger with parent, category and level.
 *
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @tparam Char Character type.
 * @param parent Parent logger to inherit sinks from.
 * @param category Logger category name.
 * @param level Logging level.
 * @return Shared pointer to the created logger.
 */
template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto create_logger(
    const std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>& parent,
    std::basic_string_view<Char> category,
    Level level) -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    using LoggerType = Logger<Char, ThreadingPolicy, BufferSize, Allocator>;
    return LoggerType::create(parent, category, level);
}

/**
 * @brief Creates a child logger with parent, category and level from string literal.
 *
 * @tparam Char Character type for the string.
 * @tparam N Size of the character array.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @param parent Parent logger to inherit sinks from.
 * @param category Logger category name.
 * @param level Logging level.
 * @return Shared pointer to the created logger.
 */
template<
    typename Char,
    std::size_t N,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
auto create_logger(
    const std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>& parent,
    const Char (&category)[N],
    Level level) -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    return create_logger<Char>(parent, category, level);
}

/**
 * @brief Creates a child logger with parent and category (level inherited from parent).
 *
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @tparam Char Character type.
 * @param parent Parent logger to inherit sinks from.
 * @param category Logger category name.
 * @return Shared pointer to the created logger.
 */
template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto create_logger(
    const std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>& parent,
    std::basic_string_view<Char> category)
    -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    using LoggerType = Logger<Char, ThreadingPolicy, BufferSize, Allocator>;
    return LoggerType::create(parent, category);
}

/**
 * @brief Creates a child logger with parent and category from string literal.
 *
 * @tparam Char Character type for the string.
 * @tparam N Size of the character array.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @param parent Parent logger to inherit sinks from.
 * @param category Logger category name.
 * @return Shared pointer to the created logger.
 */
template<
    typename Char,
    std::size_t N,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultBufferSize,
    typename Allocator = std::allocator<Char>>
auto create_logger(
    const std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>& parent,
    const Char (&category)[N])
    -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    return create_logger<Char>(parent, category);
}

/**
 * @brief Creates a child logger with parent and level (category inherited from parent).
 *
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @tparam Char Character type.
 * @param parent Parent logger to inherit sinks from.
 * @param level Logging level.
 * @return Shared pointer to the created logger.
 */
template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto create_logger(
    const std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>& parent,
    Level level) -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    using LoggerType = Logger<Char, ThreadingPolicy, BufferSize, Allocator>;
    return LoggerType::create(parent, level);
}

/**
 * @brief Creates a child logger with parent (category and level inherited from parent).
 *
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 * @tparam Char Character type.
 * @param parent Parent logger to inherit sinks from.
 * @return Shared pointer to the created logger.
 */
template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto create_logger(
    const std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>& parent)
    -> std::shared_ptr<Logger<Char, ThreadingPolicy, BufferSize, Allocator>>
{
    using LoggerType = Logger<Char, ThreadingPolicy, BufferSize, Allocator>;
    return LoggerType::create(parent);
}

/**
 * @brief Deduction guide for a constructor call with a string literal.
 *
 * @tparam Char Character type for the string.
 * @tparam N Size of the character array.
 */
template<typename Char, std::size_t N>
Logger(
    const Char (&)[N], // NOLINT(*-avoid-c-arrays)
    Level = Level::Info) -> Logger<Char>;

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/logger-inl.h" // IWYU pragma: keep
#endif
