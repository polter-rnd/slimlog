/**
 * @file policy.h
 * @brief Contains definition of SingleThreadedPolicy and MultiThreadedPolicy classes.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace PlainCloud::Log {

/**
 * @brief Single-threaded policy.
 *
 * Used to handle data manipulation without any locks or atomics.
 */
struct SingleThreadedPolicy final { };

/**
 * @brief Multi-threaded policy.
 *
 * Used to handle thread-safe data manipulation at cost of possible locking.
 *
 * @tparam Mutex Mutex type used for locking.
 * @tparam ReadLock Lock type used for read-only access.
 * @tparam WriteLock Lock type used for write access.
 * @tparam LoadOrder Memory order for loading value.
 * @tparam StoreOrder Memory order for storing new value.
 */
template<
    typename Mutex = std::shared_mutex,
    typename ReadLock = std::shared_lock<Mutex>,
    typename WriteLock = std::unique_lock<Mutex>,
    std::memory_order LoadOrder = std::memory_order_relaxed,
    std::memory_order StoreOrder = std::memory_order_relaxed>
struct MultiThreadedPolicy final { };

} // namespace PlainCloud::Log
