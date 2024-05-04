/**
 * @file level.h
 * @brief Contains definition of Level and LevelDriver classes.
 */

#pragma once

#include "policy.h"

#include <atomic>

namespace PlainCloud::Log {

/**
 * @brief Logging level.
 *
 * Specifies log event severity (e.g. `Debug`, `Info`, `Warning`)
 */
enum class Level {
    Fatal, ///< Very severe error events that will presumably lead the application to abort.
    Error, ///< Error events that might still allow the application to continue running.
    Warning, ///< Potentially harmful situations.
    Info, ///< Messages that highlight the progress of the application at coarse-grained level.
    Debug, ///< Fine-grained informational events that are most useful to debug an application.
    Trace ///< Used to "trace" entry and exiting of methods.
};

/**
 * @brief Basic log level driver class.
 *
 * Used to handle thread-safe manipulation of logging level field.
 *
 * @tparam ThreadingPolicy Threading policy used for operating over log level
 *                         (e.g. SingleThreadedPolicy or MultiThreadedPolicy).
 *
 * @note Class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class explicitly
 *       moving access functions to public part of a base class.
 */
template<typename ThreadingPolicy>
class LevelDriver final { };

/**
 * @brief Single-threaded log level driver.
 *
 * Handles log level access without any synchronization.
 */
template<>
class LevelDriver<SingleThreadedPolicy> final {
public:
    explicit LevelDriver(Level level) noexcept
        : m_level{level}
    {
    }

    /**
     * @brief Get currently enabled log level.
     *
     * @return Log level set for logger.
     */
    explicit operator Level() const noexcept
    {
        return m_level;
    }

    /**
     * @brief Set log level for logger.
     *
     * @param level New log level for logger.
     */
    auto operator=(Level level) noexcept -> auto&
    {
        m_level = level;
        return *this;
    }

private:
    Level m_level;
};

/**
 * @brief Multi-threaded log level driver.
 *
 * Handles log level access with atomic.
 */
template<
    typename Mutex,
    typename ReadLock,
    typename WriteLock,
    std::memory_order LoadOrder,
    std::memory_order StoreOrder>
class LevelDriver<MultiThreadedPolicy<Mutex, ReadLock, WriteLock, LoadOrder, StoreOrder>> final {
public:
    explicit LevelDriver(Level level) noexcept
        : m_level{level}
    {
    }

    /**
     * @brief Get currently enabled log level.
     *
     * @return Log level set for logger.
     */
    explicit operator Level() const noexcept
    {
        return m_level.load();
    }

    /**
     * @brief Set log level for logger.
     *
     * @param level  New log level for logger.
     */
    auto operator=(Level level) noexcept -> auto&
    {
        m_level.store(level);
        return *this;
    }

private:
    std::atomic<Level> m_level;
};

} // namespace PlainCloud::Log
