/**
 * @file sink.h
 * @brief Contains the definition of Sink and SinkDriver classes.
 */

#pragma once

#include "format.h"
#include "location.h"
#include "pattern.h"
#include "record.h"
#include "util/os.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <queue>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// Suppress include for std::equal_to() which is not used directly
// IWYU pragma: no_include <functional>

namespace PlainCloud::Log {

enum class Level : std::uint8_t;

/**
 * @brief Base abstract sink class.
 *
 * Sink represents a logger back-end.
 * It processes messages and forwards them to the final destination.
 *
 * @tparam Logger Logger class type intended for the sink to be used with.
 */
template<typename Logger>
class Sink : public std::enable_shared_from_this<Sink<Logger>> {
public:
    /** @brief String type for log messages. */
    using StringType = typename Logger::StringType;
    /** @brief String view type for log category. */
    using StringViewType = typename Logger::StringViewType;
    /** @brief Char type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief Buffer used for log message formatting. */
    using FormatBufferType = FormatBuffer<CharType, Logger::BufferSize, std::allocator<CharType>>;
    /** @brief Log record type. */
    using RecordType = Record<CharType, StringType>;

    /**
     * @brief Constructs a new Sink object.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(
            std::cout,
            "(%t) [%l] %F|%L: %m",
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
    explicit Sink(Args&&... args)
        // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
        : m_pattern(std::forward<Args>(args)...)
    {
    }

    /** @brief Copy constructor. */
    Sink(Sink const&) = default;
    /** @brief Move constructor. */
    Sink(Sink&&) noexcept = default;

    /** @brief Assignment operator. */
    auto operator=(Sink const&) -> Sink& = default;
    /** @brief Move assignment operator. */
    auto operator=(Sink&&) noexcept -> Sink& = default;

    /** @brief Destructor. */
    virtual ~Sink() = default;

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
     * @return Pointer to the self sink object.
     */
    virtual auto set_pattern(StringViewType pattern) -> std::shared_ptr<Sink<Logger>>
    {
        m_pattern.set_pattern(std::move(pattern));
        return this->shared_from_this();
    }

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
     * @return Pointer to the self sink object.
     */
    virtual auto set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels)
        -> std::shared_ptr<Sink<Logger>>
    {
        m_pattern.set_levels(std::move(levels));
        return this->shared_from_this();
    }

    /**
     * @brief Emits a log record.
     *
     * Message is stored in `std::variant<StringType, RecordStringView<Char>>`.
     * The former type is used for direct passing custom string type, while latter is for
     * types convertible to `std::basic_string_view<Char>`.
     *
     * @param record Log record (message, category, etc.).
     */
    virtual auto message(RecordType& record) -> void = 0;

    /**
     * @brief Flush message cache, if any.
     */
    virtual auto flush() -> void = 0;

protected:
    /**
     * @brief Formats record according to the pattern.
     *
     * Raw message is stored in \a result argument,
     * and it should be overwritten with the formatted result.
     *
     * @param result Buffer containing raw message and to be overwritten with the formatted result.
     * @param record Log record.
     */
    auto format(FormatBufferType& result, RecordType& record) -> void
    {
        m_pattern.format(result, record);
    }

private:
    Pattern<CharType> m_pattern;
};

/**
 * @brief Sink driver for the logger.
 *
 * Manages sinks with or without synchronization,
 * depending on the threading policy.
 *
 * @tparam Logger Base logger type used for log messages.
 * @tparam ThreadingPolicy Singlethreaded or multithreaded policy.
 */
template<typename Logger, typename ThreadingPolicy>
class SinkDriver final {
public:
    /** @brief Character type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief String view type for log category. */
    using StringViewType = typename Logger::StringViewType;
    /** @brief Buffer type used for log message formatting. */
    using FormatBufferType = typename Sink<Logger>::FormatBufferType;
    /** @brief Log record type. */
    using RecordType = typename Sink<Logger>::RecordType;

    /**
     * @brief Constructs a new SinkDriver object.
     *
     * @param logger Pointer to the logger.
     * @param parent Pointer to the parent instance (if any).
     */
    explicit SinkDriver(const Logger* logger, SinkDriver* parent = nullptr)
        : m_logger(logger)
        , m_parent(parent)
    {
        if (m_parent) {
            m_parent->add_child(this);
            update_effective_sinks();
        }
    }

    SinkDriver(const SinkDriver&) = delete;
    SinkDriver(SinkDriver&&) = delete;
    auto operator=(const SinkDriver&) -> SinkDriver& = delete;
    auto operator=(SinkDriver&&) -> SinkDriver& = delete;

    /**
     * @brief Destroys the SinkDriver object.
     */
    ~SinkDriver()
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        for (auto* child : m_children) {
            child->set_parent(m_parent);
        }
        if (m_parent) {
            m_parent->remove_child(this);
        }
    }

    /**
     * @brief Adds an existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually inserted.
     * @return \b false if the sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        const auto result = m_sinks.insert_or_assign(sink, true).second;
        update_effective_sinks();
        return result;
    }

    /**
     * @brief Creates and emplaces a new sink.
     *
     * @tparam T Sink type (e.g., ConsoleSink).
     * @tparam Args Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by the specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        const auto result
            = m_sinks.insert_or_assign(std::make_shared<T>(std::forward<Args>(args)...), true)
                  .first->first;
        update_effective_sinks();
        return result;
    }

    /**
     * @brief Removes a sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink was actually removed.
     * @return \b false if the sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        if (m_sinks.erase(sink) == 1) {
            update_effective_sinks();
            return true;
        }
        return false;
    }

    /**
     * @brief Enables or disables a sink for this logger.
     *
     * @param sink Pointer to the sink.
     * @param enabled Enabled flag.
     * @return \b true if the sink exists and is enabled.
     * @return \b false if the sink does not exist in this logger.
     */
    auto set_sink_enabled(const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            itr->second = enabled;
            update_effective_sinks();
            return true;
        }
        return false;
    }

    /**
     * @brief Checks if a sink is enabled.
     *
     * @param sink Pointer to the sink.
     * @return \b true if the sink is enabled.
     * @return \b false if the sink is disabled.
     */
    auto sink_enabled(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            return itr->second;
        }
        return false;
    }

    /**
     * @brief Emits a new callback-based log message if it fits the specified logging level.
     *
     * Emits a log message from the callback return value convertible to the logger string type.
     * Postpones formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost if it does not fit the current logging level.
     *
     * @tparam Logger Logger argument type.
     * @tparam T Invocable type for the callback. Deduced from the argument.
     * @tparam Args Format argument types. Deduced from the arguments.
     * @param level Logging level.
     * @param callback Log callback or message.
     * @param category Logger category.
     * @param location Caller location (file, line, function).
     * @param args Format arguments.
     */
    template<typename T, typename... Args>
    auto message(
        Level level,
        T&& callback,
        StringViewType category,
        Location location = Location::current(), // cppcheck-suppress passedByValue
        Args&&... args) const -> void
    {
        FormatBufferType buffer; // NOLINT(misc-const-correctness)
        RecordType record
            = {level,
               {location.file_name(),
                location.function_name(),
                static_cast<std::size_t>(location.line())},
               std::move(category),
               Util::OS::thread_id()};
        std::tie(record.time.local, record.time.nsec)
            = Util::OS::local_time<RecordTime::TimePoint>();

        // Flag to check that message has been evaluated
        bool evaluated = false;

        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        for (const auto& [sink, logger] : m_effective_sinks) {
            if (!logger->level_enabled(level)) [[unlikely]] {
                continue;
            }

            if (!evaluated) [[unlikely]] {
                evaluated = true;

                using BufferRefType = std::add_lvalue_reference_t<FormatBufferType>;
                if constexpr (std::is_invocable_v<T, BufferRefType, Args...>) {
                    // Callable with buffer argument: message will be stored in buffer.
                    callback(buffer, std::forward<Args>(args)...);
                    record.message = RecordStringView{buffer.data(), buffer.size()};
                } else if constexpr (std::is_invocable_v<T, Args...>) {
                    using RetType = typename std::invoke_result_t<T, Args...>;
                    if constexpr (std::is_void_v<RetType>) {
                        // Void callable without arguments: there is no message, just a callback
                        callback(std::forward<Args>(args)...);
                        break;
                    } else {
                        // Non-void callable without arguments: message is the return value
                        auto message = callback(std::forward<Args>(args)...);
                        if constexpr (std::is_convertible_v<RetType, RecordStringView<CharType>>) {
                            record.message = RecordStringView<CharType>{std::move(message)};
                        } else {
                            record.message = message;
                        }
                    }
                } else if constexpr (std::is_convertible_v<T, RecordStringView<CharType>>) {
                    // Non-invocable argument: argument is the message itself
                    // get rid of extra constructor
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    record.message = RecordStringView<CharType>{std::forward<T>(callback)};
                } else {
                    record.message = callback;
                }
            }

            sink->message(record);
        }
    }

protected:
    /**
     * @brief Returns a pointer to the parent sink (or `nullptr` if none).
     *
     * @return Pointer to the parent sink.
     */
    auto parent() -> SinkDriver*
    {
        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        return m_parent;
    }

    /**
     * @brief Sets the parent sink object.
     *
     * @param parent Pointer to the parent sink driver.
     */
    auto set_parent(SinkDriver* parent) -> void
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        m_parent = parent;
        update_effective_sinks();
    }

    /**
     * @brief Adds a child sink driver.
     *
     * @param child Pointer to the child sink driver.
     */
    auto add_child(SinkDriver* child) -> void
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        m_children.insert(child);
    }

    /**
     * @brief Removes a child sink driver.
     *
     * @param child Pointer to the child sink driver.
     */
    auto remove_child(SinkDriver* child) -> void
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        m_children.erase(child);
    }

private:
    /**
     * @brief Updates the effective sinks for the current sink driver and its children.
     */
    auto update_effective_sinks() -> void
    {
        // Queue for level order traversal
        std::queue<SinkDriver*> nodes;
        nodes.push(this);
        while (!nodes.empty()) {
            auto* node = nodes.front();
            if (node->m_parent) {
                node->m_effective_sinks = node->m_parent->m_effective_sinks;
                for (const auto& [sink, enabled] : node->m_sinks) {
                    if (enabled) {
                        node->m_effective_sinks.insert_or_assign(sink.get(), node->m_logger);
                    } else {
                        node->m_effective_sinks.erase(sink.get());
                    }
                }
            } else {
                node->m_effective_sinks.clear();
                for (const auto& [sink, enabled] : node->m_sinks) {
                    if (enabled) {
                        node->m_effective_sinks.emplace(sink.get(), node->m_logger);
                    }
                }
            }
            nodes.pop();

            for (auto* child : node->m_children) {
                nodes.push(child);
            }
        }
    }

    const Logger* m_logger;
    SinkDriver* m_parent;
    std::unordered_set<SinkDriver*> m_children;
    std::unordered_map<Sink<Logger>*, const Logger*> m_effective_sinks;
    std::unordered_map<std::shared_ptr<Sink<Logger>>, bool> m_sinks;
    mutable ThreadingPolicy::Mutex m_mutex;
}; // namespace PlainCloud::Log

} // namespace PlainCloud::Log
