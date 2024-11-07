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
 * Specifies the severity of log events.
 */
enum class Level : std::uint8_t {
    Fatal, ///< Very severe error events leading to application abort.
    Error, ///< Error events that might still allow continuation.
    Warning, ///< Potentially harmful situations.
    Info, ///< Informational messages about application progress.
    Debug, ///< Detailed debug information.
    Trace ///< Trace messages for method entry and exit.
};

/**
 * @brief Basic log level driver class.
 *
 * Handles thread-safe manipulation of the logging level field.
 *
 * @tparam ThreadingPolicy Threading policy (e.g., SingleThreadedPolicy).
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
