/**
 * @file sink.h
 * @brief Contains definition of Sink and SinkDriver classes.
 */

#pragma once

#include "format.h"
#include "location.h"
#include "pattern.h"
#include "policy.h"
#include "record.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <queue>
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
    /** @brief Log record type. */
    using RecordType = Record<CharType, StringType>;

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
     * @brief Convert from `std::string` to logger string type.
     *
     * Specialize this function for logger string type to be able
     * to use compile-time formatting for non-standard string types.
     *
     * @tparam String Type of logger string.
     * @param str Original string.
     * @return Converted string.
     */
    //[[maybe_unused]] virtual auto to_string_view(const StringType& str) -> StringViewType;

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
     * @brief Emit log record.
     *
     * Message is stored in `std::variant<StringType, RecordStringView<Char>>`.
     * The former type is used for direct passing custom string type, while latter is for
     * types convertable to `std::basic_string_view<Char>`.
     *
     * @param buffer Buffer where final message should be put.
     *               It already contains a message, feel free to overwrite it.
     * @param record Log record (message, category, etc.).
     */
    virtual auto message(FormatBufferType& buffer, RecordType& record) -> void = 0;

    /**
     * @brief Flush message cache, if any.
     */
    virtual auto flush() -> void = 0;

protected:
    /**
     * @brief Format record according to the pattern.
     *
     * Raw message is stored in \a result argument,
     * and it should be overwritten with formatted result.
     *
     * @param result Buffer containing raw message and to be overwritten with formatted result.
     * @param level Log level.
     * @param category %Logger category name.
     * @param location Caller location (file, line, function).
     */
    auto format(FormatBufferType& result, RecordType& record) -> void
    {
        m_pattern.format(result, record);
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
class SinkDriver<Logger, SingleThreadedPolicy> {
public:
    /** @brief Char type for log messages. */
    using CharType = typename Logger::CharType;
    /** @brief String view type for log category. */
    using StringViewType = typename Logger::StringViewType;
    /** @brief Buffer used for log message formatting. */
    using FormatBufferType = typename Sink<Logger>::FormatBufferType;
    /** @brief Log record type. */
    using RecordType = typename Sink<Logger>::RecordType;

    explicit SinkDriver(const Logger* logger, SinkDriver* parent = nullptr)
        : m_logger(logger)
        , m_parent(parent)
    {
        if (m_parent) {
            m_parent->add_child(this);
        }
    }

    SinkDriver(const SinkDriver&) = delete;
    SinkDriver(SinkDriver&&) = delete;
    auto operator=(const SinkDriver&) -> SinkDriver& = delete;
    auto operator=(SinkDriver&&) -> SinkDriver& = delete;

    ~SinkDriver()
    {
        if (m_parent) {
            m_parent->remove_child(this);
        }
    }

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<Logger>>& sink) -> bool
    {
        const auto result = m_sinks.insert_or_assign(sink, true).second;
        update_effective_sinks();
        return result;
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
        const auto result
            = m_sinks.insert_or_assign(std::make_shared<T>(std::forward<Args>(args)...), true)
                  .first->first;
        update_effective_sinks();
        return result;
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
        if (m_sinks.erase(sink) == 1) {
            update_effective_sinks();
            return true;
        }
        return false;
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
            update_effective_sinks();
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
        Level level,
        T&& callback,
        StringViewType category,
        Location location = Location::current(), // cppcheck-suppress passedByValue
        Args&&... args) const -> void
    {
        FormatBufferType buffer;
        RecordType record
            = {level,
               {location.file_name(), location.function_name(), location.line()},
               std::move(category)};

        // Flag to check that message has been evaluated
        bool evaluated = false;

        for (const auto& [sink, logger] : m_effective_sinks) {
            if (!logger->level_enabled(level)) {
                continue;
            }

            if (!evaluated) {
                using BufferRefType = std::add_lvalue_reference_t<FormatBufferType>;
                if constexpr (std::is_invocable_v<T, BufferRefType, Args...>) {
                    // Callable with buffer argument: message will be stored in buffer.
                    callback(buffer, std::forward<Args>(args)...);
                    record.message = RecordStringView(buffer.data(), buffer.size());
                    // Update pointer to message on every buffer re-allocation
                    buffer.on_grow(
                        [](const CharType* data, std::size_t, void* message) {
                            static_cast<RecordStringView<CharType>*>(message)->update_data_ptr(
                                data);
                        },
                        &std::get<RecordStringView<CharType>>(record.message));
                } else if constexpr (std::is_invocable_v<T, Args...>) {
                    if constexpr (std::is_void_v<typename std::invoke_result_t<T, Args...>>) {
                        // Void callable without arguments: there is no message, just a callback
                        callback(std::forward<Args>(args)...);
                        return;
                    } else {
                        // Non-void callable without arguments: message is the return value
                        auto message = callback(std::forward<Args>(args)...);
                        using Ret = std::invoke_result_t<T>;
                        if constexpr (std::is_convertible_v<Ret, RecordStringView<CharType>>) {
                            record.message = RecordStringView<CharType>{std::move(message)};
                        } else {
                            record.message = std::move(message);
                        }
                    }
                } else if constexpr (std::is_convertible_v<T, RecordStringView<CharType>>) {
                    // Non-invocable argument: argument is the message itself
                    // get rid of extra constructor
                    // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                    record.message = RecordStringView<CharType>{std::forward<T>(callback)};
                } else {
                    record.message = std::forward<T>(callback);
                }
                evaluated = true;
            }

            sink->message(buffer, record);
        }
    }

protected:
    auto parent() -> SinkDriver*
    {
        return m_parent;
    }

    auto set_parent(SinkDriver* parent) -> void
    {
        m_parent = parent;
    }

    auto add_child(SinkDriver* child) -> void
    {
        // std::wcout << "add_child\n";

        m_children.insert(child);
        update_effective_sinks();
    }

    auto remove_child(SinkDriver* child) -> void
    {
        // std::wcout << "remove_child\n";

        m_children.erase(child);
        update_effective_sinks();
    }

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

private:
    const Logger* m_logger;
    SinkDriver* m_parent;
    std::unordered_set<SinkDriver*> m_children;
    std::unordered_map<Sink<Logger>*, const Logger*> m_effective_sinks;
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
    : public SinkDriver<Logger, SingleThreadedPolicy> {
public:
    using SinkDriver<Logger, SingleThreadedPolicy>::SinkDriver;
    /** @brief String view type for log category. */
    using StringViewType = typename Logger::StringViewType;

    explicit SinkDriver(const Logger* logger, SinkDriver* parent = nullptr)
        : SinkDriver<Logger, SingleThreadedPolicy>(logger)
    {
        if (parent) {
            SinkDriver<Logger, SingleThreadedPolicy>::set_parent(parent);
            parent->add_child(this);
        }
    }

    SinkDriver(const SinkDriver&) = delete;
    SinkDriver(SinkDriver&&) = delete;
    auto operator=(const SinkDriver&) -> SinkDriver& = delete;
    auto operator=(SinkDriver&&) -> SinkDriver& = delete;

    ~SinkDriver()
    {
        if (auto* parent_ptr = static_cast<SinkDriver*>(this->parent()); parent_ptr) {
            SinkDriver<Logger, SingleThreadedPolicy>::set_parent(nullptr);
            parent_ptr->remove_child(this);
        }
    }

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
        return SinkDriver<Logger, SingleThreadedPolicy>::add_sink(sink);
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
        return SinkDriver<Logger, SingleThreadedPolicy>::template add_sink<T>(
            std::forward<Args>(args)...);
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
        return SinkDriver<Logger, SingleThreadedPolicy>::remove_sink(sink);
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
        return SinkDriver<Logger, SingleThreadedPolicy>::set_sink_enabled(sink, enabled);
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
        return SinkDriver<Logger, SingleThreadedPolicy>::sink_enabled(sink);
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
        Level level,
        T&& callback,
        StringViewType category,
        Location location = Location::current(), // cppcheck-suppress passedByValue
        Args&&... args) const -> void
    {
        ReadLock lock(m_mutex);
        SinkDriver<Logger, SingleThreadedPolicy>::message(
            level,
            std::forward<T>(callback), // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
            category,
            location,
            std::forward<Args>(args)...);
    }

protected:
    auto add_child(SinkDriver* child) -> void
    {
        WriteLock lock(m_mutex);
        // std::wcout << "!!!!!!!!!!!!!!!!!!! MULTITHREAD: add_child\n";
        SinkDriver<Logger, SingleThreadedPolicy>::add_child(child);
    }

    auto remove_child(SinkDriver* child) -> void
    {
        WriteLock lock(m_mutex);
        // std::wcout << "!!!!!!!!!!!!!!!!!!! MULTITHREAD: remove_child\n";
        SinkDriver<Logger, SingleThreadedPolicy>::remove_child(child);
    }

private:
    mutable Mutex m_mutex;
};

} // namespace PlainCloud::Log
