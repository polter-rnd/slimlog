/**
 * @file policy.h
 * @brief Defines the SingleThreadedPolicy and MultiThreadedPolicy classes.
 */

#pragma once

#include <mutex>
#include <shared_mutex>

namespace PlainCloud::Log {

/**
 * @brief Policy for single-threaded data manipulation.
 *
 * This policy handles data manipulation without using any locks or atomic operations.
 */
struct SingleThreadedPolicy final {
    /** @brief Dummy mutex. */
    struct Mutex {
        /** @brief Lock method that does nothing. */
        static void lock()
        {
        }

        /** @brief Unlock method that does nothing. */
        static void unlock()
        {
        }
    };

    using ReadLock = std::lock_guard<Mutex>; ///< Dummy read lock.
    using WriteLock = std::lock_guard<Mutex>; ///< Dummy write lock.
};

/**
 * @brief Policy for multi-threaded data manipulation.
 *
 * This policy ensures thread-safe data manipulation involving locking mechanisms.
 */
struct MultiThreadedPolicy final {
    using Mutex = std::shared_mutex; ///< Type of mutex used for locking.
    using ReadLock = std::shared_lock<Mutex>; ///< Type of lock used for read-only access.
    using WriteLock = std::unique_lock<Mutex>; ///< Type of lock used for write access.
};

} // namespace PlainCloud::Log
