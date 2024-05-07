/**
 * @file sink.h
 * @brief Contains definition of Sink and SinkDriver classes.
 */

#pragma once

#include "level.h"
#include "location.h"
#include "policy.h"

#include <atomic>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <utility>

namespace PlainCloud::Log {

/**
 * @brief Base abstract sink class.
 *
 * Sink represents a logger back-end.
 * It emits messages to final destination.
 *
 * @tparam String Base string type used for log messages.
 */
template<typename String>
class Sink : public std::enable_shared_from_this<Sink<String>> {
public:
    Sink() = default;
    Sink(Sink const&) = default;
    Sink(Sink&&) noexcept = default;
    auto operator=(Sink const&) -> Sink& = default;
    auto operator=(Sink&&) noexcept -> Sink& = default;
    virtual ~Sink() = default;

    virtual auto set_pattern(const String& pattern) -> std::shared_ptr<Sink<String>> = 0;

    virtual auto set_levels(const std::initializer_list<std::pair<Level, String>>& levels)
        -> std::shared_ptr<Sink<String>>
        = 0;

    /**
     * @brief Emit message.
     *
     * @param level Log level.
     * @param message Log message.
     * @param location Caller location (file, line, function).
     */
    virtual auto
    message(Level level, const String& category, const String& message, const Location& location)
        -> void
        = 0;

    /**
     * @brief Flush message cache, if any.
     */
    virtual auto flush() -> void = 0;
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
template<typename String>
class SinkDriver<String, SingleThreadedPolicy> final {
public:
    /**
     * @brief Construct a new SinkDriver object
     *
     * @param sinks Sinks to be added upon creation.
     */
    SinkDriver(const std::initializer_list<std::shared_ptr<Sink<String>>>& sinks = {})
    {
        m_sinks.reserve(sinks.size());
        for (const auto& sink : sinks) {
            m_sinks.emplace(sink, true);
        }
    }

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
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
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<String>>
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
    auto remove_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
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
    auto set_sink_enabled(const std::shared_ptr<Sink<String>>& sink, bool enabled) -> bool
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
    auto sink_enabled(const std::shared_ptr<Sink<String>>& sink) -> bool
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
     * @param logger %Logger object.
     * @param level Logging level.
     * @param callback Log callback.
     * @param location Caller location (file, line, function).
     */
    template<typename Logger, typename T, typename... Args>
        requires std::invocable<T, Args...>
    auto message(
        const Logger& logger,
        const Level level,
        const T& callback,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        if (static_cast<Level>(logger.m_level) < level) {
            return;
        }

        auto sinks{m_sinks};
        for (auto parent{logger.m_parent}; parent; parent = parent->m_parent) {
            if (static_cast<Level>(parent->m_level) >= level) {
                sinks.insert(parent->m_sinks.m_sinks.cbegin(), parent->m_sinks.m_sinks.cend());
            }
        }
        for (const auto& sink : sinks) {
            if (sink.second) {
                // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                sink.first->message(
                    level, logger.category(), callback(std::forward<Args>(args)...), location);
            }
        }
    }

protected:
    typename std::unordered_map<std::shared_ptr<Sink<String>>, bool>::const_iterator
    cbegin() const noexcept
    {
        return m_sinks.cbegin();
    }

    typename std::unordered_map<std::shared_ptr<Sink<String>>, bool>::const_iterator
    cend() const noexcept
    {
        return m_sinks.cend();
    }

private:
    std::unordered_map<std::shared_ptr<Sink<String>>, bool> m_sinks;
};

/**
 * @brief Multi-threaded log sink driver.
 *
 * Handles sinks access with locking a mutex.
 *
 * @tparam String Base string type used for log messages.
 */
template<
    typename String,
    typename Mutex,
    typename ReadLock,
    typename WriteLock,
    std::memory_order LoadOrder,
    std::memory_order StoreOrder>
class SinkDriver<String, MultiThreadedPolicy<Mutex, ReadLock, WriteLock, LoadOrder, StoreOrder>>
    final {
public:
    /**
     * @brief Construct a new SinkDriver object
     *
     * @param sinks Sinks to be added upon creation.
     */
    SinkDriver(const std::initializer_list<std::shared_ptr<Sink<String>>>& sinks = {})
        : m_sinks(sinks)
    {
    }

    /**
     * @brief Add existing sink.
     *
     * @param sink Pointer to the sink.
     * @return \b true if sink was actually inserted.
     * @return \b false if sink is already present in this logger.
     */
    auto add_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
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
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<String>>
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
    auto remove_sink(const std::shared_ptr<Sink<String>>& sink) -> bool
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
    auto set_sink_enabled(const std::shared_ptr<Sink<String>>& sink, bool enabled) -> bool
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
    auto sink_enabled(const std::shared_ptr<Sink<String>>& sink) -> bool
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
     * @param callback Log callback.
     * @param location Caller location (file, line, function).
     */
    template<typename Logger, typename T, typename... Args>
        requires std::invocable<T, Args...>
    auto message(
        const Logger& logger,
        const Level level,
        const T& callback,
        const Location& location = Location::current(),
        Args&&... args) const -> void
    {
        ReadLock lock(m_mutex);
        m_sinks.message(logger, level, callback, location, std::forward<Args>(args)...);
    }

private:
    mutable Mutex m_mutex;
    SinkDriver<String, SingleThreadedPolicy> m_sinks;

    friend class SinkDriver<String, SingleThreadedPolicy>;
};

} // namespace PlainCloud::Log
