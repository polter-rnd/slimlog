/**
 * @file sink-inl.h
 * @brief Contains the definition of Logger class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/logger.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/logger.h" // IWYU pragma: associated

#include <algorithm>
#include <iterator>

namespace SlimLog {

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::Logger(
    StringViewType category,
    Level level,
    TimeFunctionType time_func)
    : m_category(category) // NOLINT(*-array-to-pointer-decay,*-no-array-decay)
    , m_level(level)
{
    if (time_func != nullptr) {
        m_time_func = time_func;
    } else {
        m_time_func = []() { return std::make_pair(std::chrono::sys_seconds{}, std::size_t{}); };
    }
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::Logger(
    Level level, TimeFunctionType time_func)
    : Logger(StringViewType{DefaultCategory.data()}, level, std::move(time_func))
{
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::Logger(TimeFunctionType time_func)
    : Logger(StringViewType{DefaultCategory.data()}, Level::Info, std::move(time_func))
{
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::Logger(
    StringViewType category, Level level, Logger& parent)
    : m_category(category)
    , m_level(level)
    , m_time_func(parent.m_time_func)
    , m_parent(&parent)
{
    m_parent->add_child(this);
    update_effective_sinks();
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::Logger(
    StringViewType category, Logger& parent)
    : Logger(category, parent.level(), parent)
{
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::~Logger()
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    for (auto* child : m_children) {
        child->set_parent(m_parent);
    }
    if (m_parent) {
        m_parent->remove_child(this);
    }
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::category() const
    -> StringViewType
{
    return StringViewType{m_category};
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::add_sink(
    const std::shared_ptr<SinkType>& sink) -> bool
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    const auto result = m_sinks.insert_or_assign(sink, true).second;
    update_effective_sinks();
    return result;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::remove_sink(
    const std::shared_ptr<SinkType>& sink) -> bool
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    if (m_sinks.erase(sink) > 0) {
        update_effective_sinks();
        return true;
    }
    return false;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::set_sink_enabled(
    const std::shared_ptr<SinkType>& sink, bool enabled) -> bool
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
        itr->second = enabled;
        update_effective_sinks();
        return true;
    }
    return false;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::sink_enabled(
    const std::shared_ptr<SinkType>& sink) const -> bool
{
    const typename ThreadingPolicy::ReadLock lock(m_mutex);
    if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
        return itr->second;
    }
    return false;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::set_level(Level level) -> void
{
    m_level = level;
}

/**
 * @brief Gets the logging level.
 *
 * @return Logging level for this logger.
 */
template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
[[nodiscard]] auto // For clang-format < 19
Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::level() const -> Level
{
    return static_cast<Level>(m_level);
}

/**
 * @brief Checks if a particular logging level is enabled for the logger.
 *
 * @param level Log level to check.
 * @return \b true if the specified \p level is enabled.
 * @return \b false if the specified \p level is disabled.
 */
template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
[[nodiscard]] auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::level_enabled(
    Level level) const noexcept -> bool
{
    return static_cast<Level>(m_level) >= level;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::parent() -> Logger*
{
    const typename ThreadingPolicy::ReadLock lock(m_mutex);
    return m_parent;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::set_parent(Logger* parent)
    -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_parent = parent;
    update_effective_sinks();
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::add_child(Logger* child) -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_children.push_back(child);
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::remove_child(Logger* child)
    -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    if (auto it = std::find(m_children.begin(), m_children.end(), child); it != m_children.end()) {
        m_children.erase(it);
    }
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::update_effective_sinks(
    Logger* logger) -> Logger*
{
    typename ThreadingPolicy::ReadLock parent_lock;
    Logger* parent = logger->m_parent;
    if (parent) {
        if (parent != this) {
            // Avoid deadlock when locking parent which is already write-locked
            parent_lock = typename ThreadingPolicy::ReadLock(parent->m_mutex);
        }
        logger->m_effective_sinks = parent->m_effective_sinks;
    } else {
        logger->m_effective_sinks.clear();
    }

    // Update the current node's effective sinks
    for (const auto& [sink, enabled] : logger->m_sinks) {
        const auto it = std::find_if(
            logger->m_effective_sinks.begin(),
            logger->m_effective_sinks.end(),
            [&sink](const auto& pair) { return pair.first == sink.get(); });
        const bool found = it != logger->m_effective_sinks.end();

        if (enabled) {
            if (found) {
                it->second = logger;
            } else {
                logger->m_effective_sinks.emplace_back(sink.get(), logger);
            }
        } else if (found) {
            logger->m_effective_sinks.erase(it);
        }
    }

    // Find the next node in level order
    Logger* next = nullptr;
    if (logger->m_children.empty()) {
        // Move to the next sibling or parent's sibling
        Logger* prev = logger;
        while (parent && next == nullptr) {
            if (auto it = std::find(parent->m_children.begin(), parent->m_children.end(), prev);
                std::distance(it, parent->m_children.end()) > 1) {
                next = *std::next(it);
            }
            prev = parent;
            parent = parent->m_parent != this ? parent->m_parent : nullptr;
        }
    } else {
        // Nove to next level
        next = logger->m_children.front();
    }

    return next;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto Logger<String, Char, ThreadingPolicy, BufferSize, Allocator>::update_effective_sinks() -> void
{
    // Update current driver without lock since update_effective_sinks()
    // have to be called already under the write lock.
    Logger* next = update_effective_sinks(this);
    while (next) {
        const typename ThreadingPolicy::WriteLock next_lock(next->m_mutex);
        next = update_effective_sinks(next);
    }
}

} // namespace SlimLog
