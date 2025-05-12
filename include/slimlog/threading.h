/**
 * @file threading.h
 * @brief Defines the SingleThreadedPolicy and MultiThreadedPolicy classes
 *        as well as AtomicWrapper template.
 */

#pragma once

#include <atomic>
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
    using ReadLock = std::unique_lock<Mutex>;
    /** @brief Dummy write lock. */
    using WriteLock = std::unique_lock<Mutex>;
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

/**
 * @brief Thread-safe wrapper for values based on the threading policy.
 *
 * Generic template for wrapping values with thread-safety characteristics.
 *
 * @tparam T Type of the wrapped value
 * @tparam ThreadingPolicy Threading policy (SingleThreadedPolicy or MultiThreadedPolicy)
 */
template<typename T, typename ThreadingPolicy>
class AtomicWrapper final { };

/**
 * @brief Single-threaded specialized implementation of AtomicWrapper.
 *
 * Handles value access without any synchronization for single-threaded environments.
 *
 * @tparam T Type of the wrapped value
 */
template<typename T>
class AtomicWrapper<T, SingleThreadedPolicy> final {
public:
    /**
     * @brief Constructs a new AtomicWrapper object.
     *
     * @param value Initial value to be stored.
     */
    explicit AtomicWrapper(T value) noexcept
        : m_value{value}
    {
    }

    /**
     * @brief Gets the currently stored value.
     *
     * @return The stored value.
     */
    explicit operator T() const noexcept
    {
        return m_value;
    }

    /**
     * @brief Sets a new value.
     *
     * @param value New value to store.
     * @return Reference to the current object.
     */
    auto operator=(T value) noexcept -> auto&
    {
        m_value = value;
        return *this;
    }

private:
    T m_value;
};

/**
 * @brief Multi-threaded specialized implementation of AtomicWrapper.
 *
 * Handles value access with atomic operations for thread-safe environments.
 *
 * @tparam T Type of the wrapped value
 */
template<typename T>
class AtomicWrapper<T, MultiThreadedPolicy> final {
public:
    /**
     * @brief Constructs a new AtomicWrapper object.
     *
     * @param value Initial value to be stored.
     */
    explicit AtomicWrapper(T value) noexcept
        : m_value{value}
    {
    }

    /**
     * @brief Gets the currently stored value atomically.
     *
     * @return The stored value.
     */
    explicit operator T() const noexcept
    {
        return m_value.load(std::memory_order_relaxed);
    }

    /**
     * @brief Sets a new value atomically.
     *
     * @param value New value to store.
     * @return Reference to the current object.
     */
    auto operator=(T value) noexcept -> auto&
    {
        m_value.store(value, std::memory_order_relaxed);
        return *this;
    }

private:
    std::atomic<T> m_value;
};

} // namespace SlimLog
