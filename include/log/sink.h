/**
 * @file sink.h
 * @brief Contains definition of Sink and SinkDriver classes.
 */

#pragma once

#include "format.h"
#include "level.h"
#include "location.h"
#include "pattern.h"
#include "policy.h"
#include "util/stack_allocator.h"

#include <array>
#include <atomic>
#include <initializer_list>
#include <memory>
#include <memory_resource>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// Suppress include for std::equal_to() which is not used directly
// IWYU pragma: no_include <functional>

namespace PlainCloud::Log {

/**
 * @brief Base abstract sink class.
 *
 * Sink represents a logger back-end.
 * It processes messages and forwards them to final destination.
 *
 * @tparam Logger %Logger class type intended for the sink to be used with.
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

    /**
     * @brief Construct a new Sink object.
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
     * @brief Set the log message pattern.
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
     * @brief Set the log level names.
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
     * @brief Emit message.
     *
     * This overload is used for simple messages that can be forwarded directly to sink, e.g.:
     *
     * ```cpp
     * log.info("Hello World!");
     * ```
     *
     * Here no preprocessing takes place, the string object is directly forwarded to the sink.
     *
     * @param buffer Empty buffer where final message should be put.
     * @param level Log level.
     * @param message Log message.
     * @param category %Logger category name.
     * @param location Caller location (file, line, function).
     */
    virtual auto message(
        FormatBufferType& buffer,
        Level level,
        StringViewType category,
        StringType message,
        Location location) -> void = 0;

    /**
     * @brief Emit message.
     *
     * This overload is used for formatted messages, e.g.:
     *
     * ```cpp
     * log.info("Hello {}!", "World");
     * ```
     *
     * In this case message is formatted and written to \p buffer before being passed to the sink.
     *
     * @param buffer Buffer where final message should be put.
     *               It already contains a message, feel free to overwrite it.
     * @param level Log level.
     * @param category %Logger category name.
     * @param location Caller location (file, line, function).
     */
    virtual auto message(
        FormatBufferType& buffer, Level level, StringViewType category, Location location) -> void
        = 0;

    /**
     * @brief Flush message cache, if any.
     */
    virtual auto flush() -> void = 0;

protected:
    /**
     * @brief Apply pattern to prepare formatted message.
     *
     * Raw message is stored in \a result argument,
     * and it should be overwritten with formatted result.
     *
     * @param result Buffer containing raw message and to be overwritten with formatted result.
     * @param level Log level.
     * @param category %Logger category name.
     * @param location Caller location (file, line, function).
     */
    auto
    apply_pattern(FormatBufferType& result, Level level, StringViewType category, Location location)
        const -> void
    {
        m_pattern.apply(result, level, std::move(category), location);
    }

private:
    Pattern<CharType> m_pattern;
};

/**
 * @brief Basic log sink driver class.
 *
 * @tparam String Base string type used for log messages.
 * @tparam ThreadingPolicy Threading policy used for operating over sinks
 *                         (e.g. SingleThreadedPolicy or MultiThreadedPolicy).
 *
 * @note Class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class explicitly
 *       moving access functions to public part of a base class.
 */
template<typename String, typename ThreadingPolicy>
class SinkDriver final { };

/**
 * @brief Single-threaded log sink driver.
 *
 * Handles sinks access without any synchronization.
 *
 * @tparam String Base string type used for log messages.
 */
template<typename Logger>
class SinkDriver<Logger, SingleThreadedPolicy> final {
public:
    /** @brief Char type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief Buffer used for log message formatting. */
    using FormatBufferType = typename Sink<Logger>::FormatBufferType;
    /** @brief Arena for internal memory allocations. */
    using ArenaType = typename std::array<char, Logger::BufferSize>;

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        return m_sinks.insert_or_assign(sink, true).second;
    }

    /**
     * @brief Create and emplace a new sink.
     *
     * @tparam T %Sink type (e.g. ConsoleSink)
     * @tparam Args %Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        return m_sinks.insert_or_assign(std::make_shared<T>(std::forward<Args>(args)...), true)
            .first->first;
    }

    /**
     * @brief Remove sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually removed.
     * @return \b false if sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        return m_sinks.erase(sink) == 1;
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
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            itr->second = enabled;
            return true;
        }
        return false;
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
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            return itr->second;
        }
        return false;
    }

    /**
     * @brief Emit new callback-based log message if it fits for specified logging level.
     *
     * Method to emit log message from callback return value convertible to logger string type.
     * Used to postpone formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost in case if it does not fit for current logging level.
     *
     * @tparam Logger %Logger argument type
     * @tparam T Invocable type for the callback. Deduced from argument.
     * @tparam Args Format argument types. Deduced from arguments.
     * @param arena Continious space for internal memory allocations.
     * @param logger %Logger object.
     * @param level Logging level.
     * @param callback Log callback or message.
     * @param location Caller location (file, line, function).
     * @param args Format arguments.
     */
    template<typename T, typename... Args>
    auto message(
        const Logger& logger,
        Level level,
        T&& callback,
        Location location = Location::current(),
        Args&&... args) const -> void
    {
        /*std::unordered_map<
            Sink<Logger>*,
            bool,
            std::hash<Sink<Logger>*>,
            std::equal_to<Sink<Logger>*>,
            StackAllocator<std::pair<Sink<Logger>* const, bool>, Logger::BufferSize>>
            sinks{alloc<std::pair<Sink<Logger>* const, bool>>()};*/
        /*std::unordered_map<
            Sink<Logger>*,
            bool,
            std::hash<Sink<Logger>*>,
            std::equal_to<Sink<Logger>*>>
            sinks;

        if (logger.level_enabled(level)) {
            for (auto it = m_sinks.cbegin(), it_end = m_sinks.cend(); it != it_end; it++) {
                sinks.emplace(it->first.get(), it->second);
            }
        }
        for (auto parent{logger.m_parent}; parent; parent = parent->m_parent) {
            if (parent->level_enabled(level)) {
                for (auto it = parent->m_sinks.m_sinks.cbegin(),
                          it_end = parent->m_sinks.m_sinks.cend();
                     it != it_end;
                     it++) {
                    sinks.emplace(it->first.get(), it->second);
                }
            }
        }*/
        const auto& sinks = m_sinks;
        FormatBufferType& fmt_buffer = buffer();

        using BufferRefType = std::add_lvalue_reference_t<FormatBufferType>;
        if constexpr (std::is_invocable_v<T, BufferRefType, Args...>) {
            callback(fmt_buffer, std::forward<Args>(args)...);
            for (const auto& sink : sinks) {
                if (sink.second) {
                    sink.first->message(fmt_buffer, level, logger.category(), location);
                }
            }
        } else if constexpr (std::is_invocable_v<T, Args...>) {
            if constexpr (std::is_void_v<typename std::invoke_result_t<T, Args...>>) {
                callback(std::forward<Args>(args)...);
                for (const auto& sink : sinks) {
                    if (sink.second) {
                        sink.first->message(fmt_buffer, level, logger.category(), location);
                    }
                }
            } else {
                auto message = callback(std::forward<Args>(args)...);
                for (const auto& sink : sinks) {
                    if (sink.second) {
                        sink.first->message(
                            fmt_buffer, level, logger.category(), message, location);
                    }
                }
            }
        } else {
            for (const auto& sink : sinks) {
                if (sink.second) {
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    sink.first->message(
                        fmt_buffer, level, logger.category(), std::forward<T>(callback), location);
                }
            }
        }

        buffer().clear();
    }

protected:
    /**
     * @brief Static-allocated continious space for memory allocations.
     *
     * @return Reference to the arena.
     */

    [[nodiscard]] auto buffer() const -> auto&
    {
        static FormatBufferType fmt_buffer;
        return fmt_buffer;
    }

    /**
     * @brief Constant begin interator for the sinks map.
     */
    auto cbegin() const noexcept
    {
        return m_sinks.cbegin();
    }

    /**
     * @brief Constant end interator for the sinks map.
     */
    auto cend() const noexcept
    {
        return m_sinks.cend();
    }

private:
    std::unordered_map<std::shared_ptr<Sink<Logger>>, bool> m_sinks;
};

/**
 * @brief Multi-threaded log sink driver.
 *
 * Handles sinks access with locking a mutex.
 *
 * @tparam String Base string type used for log messages.
 */
template<
    typename Logger,
    typename Mutex,
    typename ReadLock,
    typename WriteLock,
    std::memory_order LoadOrder,
    std::memory_order StoreOrder>
class SinkDriver<Logger, MultiThreadedPolicy<Mutex, ReadLock, WriteLock, LoadOrder, StoreOrder>>
    final {
public:
    /** @brief Char type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief Buffer used for log message formatting. */
    using FormatBufferType = typename SinkDriver<Logger, SingleThreadedPolicy>::FormatBufferType;
    /** @brief Arena for internal memory allocations. */
    using ArenaType = typename std::array<char, Logger::BufferSize>;

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        WriteLock lock(m_mutex);
        return m_sinks.add_sink(sink);
    }

    /**
     * @brief Create and emplace a new sink.
     *
     * @tparam T %Sink type (e.g. ConsoleSink)
     * @tparam Args %Sink constructor argument types (deduced from arguments).
     * @param args Any arguments accepted by specified sink constructor.
     * @return Pointer to the created sink.
     */
    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<Logger>>
    {
        WriteLock lock(m_mutex);
        return m_sinks.template add_sink<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Remove sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually removed.
     * @return \b false if sink does not exist in this logger.
     */
    auto remove_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        WriteLock lock(m_mutex);
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
        WriteLock lock(m_mutex);
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
        ReadLock lock(m_mutex);
        return m_sinks.sink_enabled(sink);
    }

    /**
     * @brief Emit new callback-based log message if it fits for specified logging level.
     *
     * Method to emit log message from callback return value convertible to logger string type.
     * Used to postpone formatting or other preparations to the next steps after filtering.
     * Makes logging almost zero-cost in case if it does not fit for current logging level.
     *
     * @tparam Logger %Logger argument type
     * @tparam T Invocable type for the callback. Deduced from argument.
     * @tparam Args Format argument types. Deduced from arguments.
     * @param logger %Logger object.
     * @param level Logging level.
     * @param callback Log callback or message.
     * @param location Caller location (file, line, function).
     * @param args Format arguments.
     */
    template<typename T, typename... Args>
    auto message(
        const Logger& logger,
        Level level,
        T&& callback,
        Location location = Location::current(),
        Args&&... args) const -> void
    {
        ReadLock lock(m_mutex);
        m_sinks.message(
            logger,
            level,
            std::forward<T>(callback), // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
            location,
            std::forward<Args>(args)...);
    }

protected:
    /**
     * @brief Static-allocated thread-local continious space for memory allocations.
     *
     * @return Reference to the arena.
     */
    [[nodiscard]] auto buffer() const -> auto&
    {
        static thread_local FormatBufferType fmt_buffer;
        return fmt_buffer;
    }

private:
    mutable Mutex m_mutex;
    SinkDriver<Logger, SingleThreadedPolicy> m_sinks;

    friend class SinkDriver<Logger, SingleThreadedPolicy>;
};

} // namespace PlainCloud::Log
