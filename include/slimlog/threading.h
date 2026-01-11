/**
 * @file threading.h
 * @brief Defines the SingleThreadedPolicy and MultiThreadedPolicy classes
 *        as well as AtomicWrapper template.
 */

#pragma once

#include <atomic>
#include <concepts>
#include <mutex>
#include <shared_mutex>

namespace slimlog {

/**
 * @brief Policy for single-threaded data manipulation.
 *
 * This policy handles data manipulation without using any locks or atomic operations.
 */
struct SingleThreadedPolicy final {
    /** @brief Dummy mutex (expands to nothing). */
    struct Mutex {};

    /** @brief Dummy shared mutex. */
    using SharedMutex = Mutex;

    /** @brief Dummy write lock. */
    template<typename MutexType>
    struct UniqueLock {
        /** @brief Constructs a new dummy UniqueLock object. */
        explicit UniqueLock(MutexType& /*unused*/)
        {
        }
    };

    /** @brief Dummy read lock. */
    template<typename MutexType>
    struct SharedLock {
        /** @brief Constructs a new dummy SharedLock object. */
        explicit SharedLock(MutexType& /*unused*/)
        {
        }
    };
};

/**
 * @brief Policy for multi-threaded data manipulation.
 *
 * This policy ensures thread-safe data manipulation involving locking mechanisms.
 */
struct MultiThreadedPolicy final {
    /** @brief Mutex type for synchronization. */
    using Mutex = std::mutex;

    /** @brief SharedMutex type for synchronization. */
    using SharedMutex = std::shared_mutex;

    /** @brief Write lock wrapper with CTAD support. */
    template<typename MutexType>
    struct UniqueLock : std::unique_lock<MutexType> {
        /**
         * @brief Constructs a new UniqueLock object.
         * @param mutex Mutex to lock for exclusive access.
         */
        explicit UniqueLock(MutexType& mutex)
            : std::unique_lock<MutexType>(mutex)
        {
        }
    };

    /** @brief Read lock wrapper with CTAD support. */
    template<typename MutexType>
    struct SharedLock : std::shared_lock<MutexType> {
        /**
         * @brief Constructs a new SharedLock object.
         * @param mutex Mutex to lock for shared access.
         */
        explicit SharedLock(MutexType& mutex)
            : std::shared_lock<MutexType>(mutex)
        {
        }
    };
};

/**
 * @brief Checks if a type is a valid threading policy.
 *
 * @tparam T Type to check.
 */
template<typename T>
concept IsThreadingPolicy
    = std::same_as<T, SingleThreadedPolicy> || std::same_as<T, MultiThreadedPolicy>;

/**
 * @brief Thread-safe wrapper for values based on the threading policy.
 *
 * Generic template for wrapping values with thread-safety characteristics.
 *
 * @tparam T Type of the wrapped value
 * @tparam ThreadingPolicy Threading policy (SingleThreadedPolicy or MultiThreadedPolicy)
 */
template<typename T, typename ThreadingPolicy>
class AtomicWrapper final {};

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
    [[nodiscard]] explicit operator T() const noexcept
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
    [[nodiscard]] explicit operator T() const noexcept
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

} // namespace slimlog
