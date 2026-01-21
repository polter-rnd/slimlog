/**
 * @file sink.h
 * @brief Contains the declaration of Sink and FormattableSink classes.
 */

#pragma once

#include "slimlog/common.h"
#include "slimlog/format.h"
#include "slimlog/pattern.h"
#include "slimlog/threading.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>
#include <utility>

// IWYU pragma: no_include <string>

namespace slimlog {

/**
 * @brief Base abstract sink class.
 *
 * A sink is a destination for log messages.
 *
 * @tparam Char Character type for the string.
 */
template<typename Char>
class Sink {
public:
    /** @brief Log record type. */
    using RecordType = Record<Char>;
    /** @brief Raw string view type. */
    using StringViewType = std::basic_string_view<Char>;

    /** @brief Default constructor. */
    Sink() = default;
    /** @brief Copy constructor. */
    Sink(const Sink&) = default;
    /** @brief Move constructor. */
    Sink(Sink&&) noexcept = default;

    /** @brief Assignment operator. */
    auto operator=(const Sink&) -> Sink& = default;
    /** @brief Move assignment operator. */
    auto operator=(Sink&&) noexcept -> Sink& = default;

    /** @brief Destructor. */
    virtual ~Sink() = default;

    /**
     * @brief Processes a log record.
     *
     * Formats and outputs the log record.
     *
     * @param record The log record to process.
     */
    // NOLINTNEXTLINE(portability-template-virtual-member-function)
    virtual auto message(const RecordType& record) -> void = 0;

    /**
     * @brief Flushes any buffered log messages.
     */
    // NOLINTNEXTLINE(portability-template-virtual-member-function)
    virtual auto flush() -> void = 0;
};

/**
 * @brief Abstract formattable sink class.
 *
 * A sink that supports custom message formatting.
 * Allocates an internal buffer for formatting log messages.
 * Ensures thread-safe operations on the format pattern.
 *
 * @tparam Char Character type for the string.
 * @tparam ThreadingPolicy Threading policy for sink operations.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename Char,
    typename ThreadingPolicy = DefaultThreadingPolicy,
    std::size_t BufferSize = DefaultSinkBufferSize,
    typename Allocator = std::allocator<Char>>
class FormattableSink : public Sink<Char> {
public:
    using typename Sink<Char>::RecordType;
    using typename Sink<Char>::StringViewType;

    /** @brief Buffer type used for log message formatting. */
    using FormatBufferType = FormatBuffer<Char, BufferSize, Allocator>;
    /** @brief Time function type for getting the current time. */
    using TimeFunctionType = typename Pattern<Char>::TimeFunctionType;

    /**
     * @brief Constructs a new Sink object.
     *
     * Accepts optional pattern and log level arguments to customize message formatting.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("main", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(
            std::cout,
            "({category}) [{level}] {file}|{line}: {message}",
            std::make_pair(Log::Level::Trace, "Trace"),
            std::make_pair(Log::Level::Debug, "Debug"),
            std::make_pair(Log::Level::Info, "Info"),
            std::make_pair(Log::Level::Warning, "Warn"),
            std::make_pair(Log::Level::Error, "Error"),
            std::make_pair(Log::Level::Fatal, "Fatal"));
     * ```
     *
     * @tparam Args Argument types for the pattern and log levels.
     * @param args Optional pattern and variadic list of log level pairs.
     */
    template<typename... Args>
    explicit FormattableSink(Args&&... args)
        // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
        : m_pattern(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Sets the time function used for log timestamps.
     * @param time_func Time function to be set for this logger.
     */
    SLIMLOG_EXPORT auto set_time_func(TimeFunctionType time_func) -> void;

    /**
     * @brief Sets the log message pattern.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_pattern("(%t) [%l] %F|%L: %m");
     * ```
     *
     * @param pattern Log message pattern.
     */
    SLIMLOG_EXPORT auto set_pattern(StringViewType pattern) -> void;

    /**
     * @brief Sets the log level names with automatic type deduction.
     *
     * This function can handle both initializer lists and variadic arguments.
     *
     * Usage examples:
     * ```cpp
     * // With initializer list:
     * sink->set_levels({{Log::Level::Info, "Information"}});
     *
     * // With variadic arguments:
     * sink->set_levels(
     *     std::make_pair(Log::Level::Info, "CUSTOM_INFO"),
     *     std::make_pair(Log::Level::Debug, "CUSTOM_DEBUG")
     * );
     * ```
     *
     * @param pairs List of log levels with corresponding names.
     */
    template<typename... Pairs>
    auto set_levels(Pairs&&... pairs) -> void
    {
        const typename ThreadingPolicy::template UniqueLock<decltype(m_mutex)> lock(m_mutex);
        m_pattern.set_levels(std::forward<Pairs>(pairs)...);
    }

protected:
    /**
     * @brief Formats a log record according to the pattern.
     *
     * @param result Buffer to store the formatted message.
     * @param record The log record to format.
     */
    auto format(FormatBufferType& result, const RecordType& record) -> void;

private:
    Pattern<Char> m_pattern;
    mutable typename ThreadingPolicy::SharedMutex m_mutex;
};

/**
 * @brief Checks if a sink template can be instantiated as a sink.
 *
 * This concept checks whether a template template parameter can be instantiated
 * and the result derives from Sink.
 *
 * @tparam T Template template parameter to check.
 * @tparam Char Character type.
 * @tparam SinkTemplateArgs Additional template arguments for the sink.
 */
template<template<typename, typename...> typename T, typename Char, typename... SinkTemplateArgs>
concept IsSink = requires {
    typename T<Char, SinkTemplateArgs...>;
    requires std::derived_from<T<Char, SinkTemplateArgs...>, Sink<Char>>;
};

/**
 * @brief Checks if a sink template can be instantiated as a formattable sink.
 *
 * This concept checks whether a template template parameter can accept the standard
 * FormattableSink template arguments (Char, ThreadingPolicy, BufferSize, Allocator).
 *
 * @tparam T Template template parameter to check.
 * @tparam Char Character type.
 * @tparam ThreadingPolicy Threading policy type.
 * @tparam BufferSize Size of the internal buffer.
 * @tparam Allocator Allocator type.
 * @tparam SinkTemplateArgs Additional template arguments for the sink.
 */
template<
    template<typename, typename, std::size_t, typename, typename...> // For clang-format < 19
    typename T,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator,
    typename... SinkTemplateArgs>
concept IsFormattableSink = requires {
    typename T<Char, ThreadingPolicy, BufferSize, Allocator, SinkTemplateArgs...>;
    requires std::derived_from<
        T<Char, ThreadingPolicy, BufferSize, Allocator, SinkTemplateArgs...>,
        FormattableSink<Char, ThreadingPolicy, BufferSize, Allocator>>;
};

/**
 * @brief Checks if a sink template can be instantiated with threading policy.
 *
 * This concept checks whether a template template parameter can accept a threading policy
 * as its second template argument, useful for distinguishing between sinks that are
 * threading-aware (like CallbackSink) and those that aren't (like NullSink).
 *
 * @tparam T Template template parameter to check.
 * @tparam Char Character type.
 * @tparam SinkTemplateArgs Additional template arguments for the sink.
 */
template<template<typename, typename...> typename T, typename Char, typename... SinkTemplateArgs>
concept IsThreadAwareSink
    = (requires {
          typename T<Char, DefaultThreadingPolicy, SinkTemplateArgs...>;
          requires IsSink<T, Char, DefaultThreadingPolicy, SinkTemplateArgs...>;
      })
    || (sizeof...(SinkTemplateArgs) > 0
        && IsThreadingPolicy<std::tuple_element_t<0, std::tuple<SinkTemplateArgs...>>>);

} // namespace slimlog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sink-inl.h" // IWYU pragma: keep
#endif
