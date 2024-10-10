/**
 * @file level.h
 * @brief Contains definitions of Level and LevelDriver classes.
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace PlainCloud::Log {

struct MultiThreadedPolicy;
struct SingleThreadedPolicy;

/**
 * @brief Logging level enumeration.
 *
 * Specifies log event severity (e.g., `Debug`, `Info`, `Warning`).
 */
enum class Level : std::uint8_t {
    Fatal, ///< Very severe error events that will presumably lead the application to abort.
    Error, ///< Error events that might still allow the application to continue running.
    Warning, ///< Potentially harmful situations.
    Info, ///< Messages that highlight the progress of the application at a coarse-grained level.
    Debug, ///< Fine-grained informational events that are most useful to debug an application.
    Trace ///< Used to "trace" entry and exiting of methods.
};

/**
 * @brief Basic log level driver class.
 *
 * Used to handle thread-safe manipulation of the logging level field.
 *
 * @tparam ThreadingPolicy Threading policy used for operating over the log level
 *                         (e.g., SingleThreadedPolicy or MultiThreadedPolicy).
 *
 * @note This class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class, explicitly
 *       moving access functions to the public part of a base class.
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
    /**
     * @brief Constructs a new LevelDriver object.
     *
     * @param level Log level to be set upon initialization.
     */
    explicit LevelDriver(Level level) noexcept
        : m_level{level}
    {
    }

    /**
     * @brief Gets the currently enabled log level.
     *
     * @return Log level set for the logger.
     */
    explicit operator Level() const noexcept
    {
        return m_level;
    }

    /**
     * @brief Sets the log level for the logger.
     *
     * @param level New log level for the logger.
     * @return Reference to the current object.
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
 * Handles log level access with atomic operations.
 */
template<>
class LevelDriver<MultiThreadedPolicy> final {
public:
    /**
     * @brief Constructs a new LevelDriver object.
     *
     * @param level Log level to be set upon initialization.
     */
    explicit LevelDriver(Level level) noexcept
        : m_level{level}
    {
    }

    /**
     * @brief Gets the currently enabled log level.
     *
     * @return Log level set for the logger.
     */
    explicit operator Level() const noexcept
    {
        return m_level.load(std::memory_order_relaxed);
    }

    /**
     * @brief Sets the log level for the logger.
     *
     * @param level New log level for the logger.
     * @return Reference to the current object.
     */
    auto operator=(Level level) noexcept -> auto&
    {
        m_level.store(level, std::memory_order_relaxed);
        return *this;
    }

private:
    std::atomic<Level> m_level;
};

} // namespace PlainCloud::Log
