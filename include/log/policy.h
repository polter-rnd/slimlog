/**
 * @file policy.h
 * @brief Defines the SingleThreadedPolicy and MultiThreadedPolicy classes.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace PlainCloud::Log {

/**
 * @brief Policy for single-threaded data manipulation.
 *
 * This policy handles data manipulation without using any locks or atomic operations.
 */
struct SingleThreadedPolicy final { };

/**
 * @brief Policy for multi-threaded data manipulation.
 *
 * This policy ensures thread-safe data manipulation, potentially involving locking mechanisms.
 *
 * @tparam Mutex Type of mutex used for locking.
 * @tparam ReadLock Type of lock used for read-only access.
 * @tparam WriteLock Type of lock used for write access.
 * @tparam LoadOrder Memory order used for loading values.
 * @tparam StoreOrder Memory order used for storing new values.
 */
template<
    typename Mutex = std::shared_mutex,
    typename ReadLock = std::shared_lock<Mutex>,
    typename WriteLock = std::unique_lock<Mutex>,
    std::memory_order LoadOrder = std::memory_order_relaxed,
    std::memory_order StoreOrder = std::memory_order_relaxed>
struct MultiThreadedPolicy final { };

} // namespace PlainCloud::Log
