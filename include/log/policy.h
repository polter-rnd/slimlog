/**
 * @file policy.h
 * @brief Defines the SingleThreadedPolicy and MultiThreadedPolicy classes.
 */

#pragma once

#include <mutex>
#include <shared_mutex>

namespace SlimLog {

/**
 * @brief Policy for single-threaded data manipulation.
 *
 * This policy handles data manipulation without using any locks or atomic operations.
 */
struct SingleThreadedPolicy final {
    /** @brief Dummy mutex (expands to nothing). */
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

    /** @brief Dummy read lock. */
    using ReadLock = std::lock_guard<Mutex>;
    /** @brief Dummy write lock. */
    using WriteLock = std::lock_guard<Mutex>;
};

/**
 * @brief Policy for multi-threaded data manipulation.
 *
 * This policy ensures thread-safe data manipulation involving locking mechanisms.
 */
struct MultiThreadedPolicy final {
    /** @brief Mutex type for synchronization. */
    using Mutex = std::shared_mutex;
    /** @brief Read lock type for shared access. */
    using ReadLock = std::shared_lock<Mutex>;
    /** @brief Write lock type for exclusive access. */
    using WriteLock = std::unique_lock<Mutex>;
};

} // namespace SlimLog
