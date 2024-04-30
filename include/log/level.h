#pragma once

#include "policy.h"

#include <atomic>
#include <mutex>

namespace PlainCloud::Log {

enum class Level { Fatal, Error, Warning, Info, Debug, Trace };

template<typename ThreadingPolicy>
class LevelDriver { };

/**
 * @brief A single thread log level driver.
 *
 * Handles log level access without any synchronization.
 *
 * @note Class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class explicitly
 *       moving access functions to public part of a base class.
 */
template<>
class LevelDriver<SingleThreadedPolicy> {
public:
    explicit LevelDriver(Level level) noexcept
        : m_level{level}
    {
    }

    // No virtual destructor as the data is trivial.

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
     * @param level  New log level for logger.
     */
    auto operator=(Level level) noexcept -> auto&
    {
        m_level = level;
        return *this;
    }

private:
    Level m_level;
};

//
// LevelDriverMT
//

/**
 * @brief A multi thread log level driver.
 *
 * Handles log level access with atomic.
 *
 * @note Class doesn't have a virtual destructor
 *       as the intended usage scenario is to
 *       use it as a private base class explicitly
 *       moving access functions to public part of a base class.
 *
 * @tparam Load_MO  Memory order for loading value.
 * @tparam Store_MO  Memory order for storing new value.
 */

template<
    typename MutexT,
    typename ReadLockT,
    typename WriteLockT,
    std::memory_order LoadOrder,
    std::memory_order StoreOrder>
class LevelDriver<MultiThreadedPolicy<MutexT, ReadLockT, WriteLockT, LoadOrder, StoreOrder>> {
public:
    explicit LevelDriver(Level level) noexcept
        : m_level{level}
    {
    }

    // No virtual destructor as the data is trivial.

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