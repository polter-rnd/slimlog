/**
 * @file sink.h
 * @brief Contains the declaration of Sink and FormattableSink classes.
 */

#pragma once

#include "slimlog/format.h"
#include "slimlog/pattern.h"
#include "slimlog/record.h"
#include "slimlog/threading.h"
#include "slimlog/util/types.h"

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <utility>

// IWYU pragma: no_include <string>

namespace SlimLog {

/**
 * @brief Default buffer size for raw log messages.
 */
static constexpr auto DefaultBufferSize = 128U;

/**
 * @brief Default per-sink buffer size for formatted log messages.
 */
static constexpr auto DefaultSinkBufferSize = 256U;

/**
 * @brief Default threading policy for logger sinks.
 */
using DefaultThreadingPolicy = SingleThreadedPolicy;

/**
 * @brief Base abstract sink class.
 *
 * A sink is a destination for log messages.
 *
 * @tparam String String type for log messages.
 * @tparam Char Character type for the string.
 */
template<typename String, typename Char = Util::Types::UnderlyingCharType<String>>
class Sink {
public:
    /** @brief Log record type. */
    using RecordType = Record<String, Char>;

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
    virtual auto message(RecordType& record) -> void = 0;

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
 * Allocates another buffer for message formatting.
 *
 * @tparam String String type for log messages.
 * @tparam Char Character type for the string.
 * @tparam BufferSize Size of the internal pre-allocated buffer.
 * @tparam Allocator Allocator type for the internal buffer.
 */
template<
    typename String,
    typename Char = Util::Types::UnderlyingCharType<String>,
    std::size_t BufferSize = DefaultSinkBufferSize,
    typename Allocator = std::allocator<Char>>
class FormattableSink : public Sink<String, Char> {
public:
    /** @brief Raw string view type. */
    using StringViewType = std::basic_string_view<Char>;
    /** @brief Buffer type used for log message formatting. */
    using FormatBufferType = FormatBuffer<Char, BufferSize, Allocator>;
    /** @brief Log record type. */
    using RecordType = Record<String, Char>;

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
     * @param args Optional pattern and list of log levels.
     */
    template<typename... Args>
    explicit FormattableSink(Args&&... args)
        // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
        : m_pattern(std::forward<Args>(args)...)
    {
    }

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
    virtual auto set_pattern(StringViewType pattern) -> void;

    /**
     * @brief Sets the log level names.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_levels({{Log::Level::Info, "Information"}});
     * ```
     *
     * @param levels List of log levels with corresponding names.
     */
    virtual auto set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels) -> void;

protected:
    /**
     * @brief Formats a log record according to the pattern.
     *
     * @param result Buffer to store the formatted message.
     * @param record The log record to format.
     */
    auto format(FormatBufferType& result, RecordType& record) -> void;

private:
    Pattern<Char> m_pattern;
};

/**
 * @brief Checks if the specified type is a formattable sink.
 *
 * @tparam T Type to check.
 */
template<class T>
concept IsFormattableSink = requires(const T& arg) {
    []<typename String, typename Char, std::size_t BufferSize, typename Allocator>(
        const FormattableSink<String, Char, BufferSize, Allocator>&) {
        /* For clang-format < 19 */
    }(arg);
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/sink-inl.h" // IWYU pragma: keep
#endif
