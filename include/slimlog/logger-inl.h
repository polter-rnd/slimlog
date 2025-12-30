/**
 * @file logger-inl.h
 * @brief Contains the definition of Logger class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/logger.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/logger.h" // IWYU pragma: associated

#include <algorithm>
#include <unordered_set>

namespace SlimLog {

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
Logger<Char, ThreadingPolicy, BufferSize, Allocator>::Logger(
    Key /*key*/, StringViewType category, Level level)
    : m_category(category)
    , m_level(level)
    , m_propagate(true)
    , m_parent(nullptr)
{
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
Logger<Char, ThreadingPolicy, BufferSize, Allocator>::Logger(Key key, Level level)
    : Logger(key, StringViewType{DefaultCategory.data(), DefaultCategory.size()}, level)
{
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::create(
    StringViewType category, Level level) -> std::shared_ptr<Logger>
{
    return std::make_shared<Logger>(Key{}, category, level);
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::create(Level level)
    -> std::shared_ptr<Logger>
{
    return std::make_shared<Logger>(Key{}, level);
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::create(
    const std::shared_ptr<Logger>& parent, // For clang-format < 19
    StringViewType category,
    Level level) -> std::shared_ptr<Logger>
{
    auto logger = std::make_shared<Logger>(Key{}, category, level);
    if (parent) {
        logger->set_parent(parent);
    }
    return logger;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::create(
    const std::shared_ptr<Logger>& parent, StringViewType category) -> std::shared_ptr<Logger>
{
    std::shared_ptr<Logger> logger;
    if (parent) {
        logger = std::make_shared<Logger>(Key{}, category, parent->level());
        logger->set_parent(parent);
    } else {
        logger = create(category);
    }
    return logger;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::create(
    const std::shared_ptr<Logger>& parent, Level level) -> std::shared_ptr<Logger>
{
    std::shared_ptr<Logger> logger;
    if (parent) {
        logger = std::make_shared<Logger>(Key{}, parent->category(), level);
        logger->set_parent(parent);
    } else {
        logger = create(level);
    }
    return logger;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::create(
    const std::shared_ptr<Logger>& parent) -> std::shared_ptr<Logger>
{
    std::shared_ptr<Logger> logger;
    if (parent) {
        logger = std::make_shared<Logger>(Key{}, parent->category(), parent->level());
        logger->set_parent(parent);
    } else {
        logger = create();
    }
    return logger;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::category() const -> StringViewType
{
    return m_category;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::add_sink(
    const std::shared_ptr<SinkType>& sink) -> bool
{
    bool added = false;
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        added = m_sinks.insert_or_assign(sink, true).second;
    }
    if (added) {
        update_propagated_sinks();
    }
    return added;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::remove_sink(
    const std::shared_ptr<SinkType>& sink) -> bool
{

    bool erased = false;
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        erased = m_sinks.erase(sink) > 0;
    }
    if (erased) {
        update_propagated_sinks();
    }
    return erased;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::set_sink_enabled(
    const std::shared_ptr<SinkType>& sink, bool enabled) -> bool
{
    bool found = false;
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            if (itr->second == enabled) {
                return true;
            }
            itr->second = enabled;
            found = true;
        }
    }
    if (found) {
        update_propagated_sinks();
    }
    return found;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::sink_enabled(
    const std::shared_ptr<SinkType>& sink) const -> bool
{
    const typename ThreadingPolicy::ReadLock lock(m_mutex);
    if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
        return itr->second;
    }
    return false;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::set_propagate(bool enabled) -> void
{
    m_propagate = enabled;
    update_propagated_sinks();
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::set_level(Level level) -> void
{
    m_level = level;
}

/**
 * @brief Gets the logging level.
 *
 * @return Logging level for this logger.
 */
template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
[[nodiscard]] auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::level() const -> Level
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
template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
[[nodiscard]] auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::level_enabled(
    Level level) const noexcept -> bool
{
    return static_cast<Level>(m_level) >= level;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::parent() -> std::shared_ptr<Logger>
{
    const typename ThreadingPolicy::ReadLock lock(m_mutex);
    return m_parent;
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::set_parent(
    const std::shared_ptr<Logger>& parent) -> void
{
    std::shared_ptr<Logger> old_parent;
    {
        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        old_parent = m_parent;
    }
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        m_parent = parent;
    }

    const std::shared_ptr<Logger> self = this->shared_from_this();
    if (old_parent) {
        old_parent->remove_child(self);
    }
    if (parent) {
        parent->add_child(self);
    }

    update_propagated_sinks();
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::add_child(
    const std::shared_ptr<Logger>& child) -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_children.push_back(child);
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::remove_child(
    const std::shared_ptr<Logger>& child) -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_children.erase(
        std::remove_if(
            m_children.begin(),
            m_children.end(),
            [&child](const std::weak_ptr<Logger>& weak_child) {
                auto locked_child = weak_child.lock();
                return !locked_child || locked_child == child;
            }),
        m_children.end());
}

template<typename Char, typename ThreadingPolicy, std::size_t BufferSize, typename Allocator>
// NOLINTNEXTLINE(misc-no-recursion)
auto Logger<Char, ThreadingPolicy, BufferSize, Allocator>::update_propagated_sinks(
    std::unordered_set<Logger*> visited) -> void
{
    // Check for cycle to prevent infinite recursion
    if (visited.find(this) != visited.end()) {
        return;
    }
    visited.insert(this);

    // Snapshot parent
    std::shared_ptr<Logger> parent;
    {
        const typename ThreadingPolicy::ReadLock lock(m_mutex);
        parent = m_parent;
    }

    // Snapshot propagated sinks
    std::vector<SinkType*> propagated_sinks;
    if (parent && m_propagate) {
        const typename ThreadingPolicy::ReadLock lock(parent->m_mutex);
        propagated_sinks = parent->m_propagated_sinks;
    }

    // Update the current node's propagated sinks, snapshot children
    std::vector<std::shared_ptr<Logger>> children;
    {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        m_propagated_sinks = propagated_sinks;
        for (const auto& [sink, enabled] : m_sinks) {
            const auto it
                = std::find(m_propagated_sinks.begin(), m_propagated_sinks.end(), sink.get());
            const auto found = it != m_propagated_sinks.end();
            if (!found && enabled) {
                // Add sink if not already present
                m_propagated_sinks.push_back(sink.get());
            } else if (found && !enabled) {
                // Remove sink if present
                m_propagated_sinks.erase(it);
            }
        }

        // Clean up expired children and snapshot valid ones
        children.reserve(m_children.size());
        m_children.erase(
            std::remove_if(
                m_children.begin(),
                m_children.end(),
                [&children](const std::weak_ptr<Logger>& weak_child) {
                    auto child = weak_child.lock();
                    if (child) {
                        children.push_back(child);
                        return false;
                    }
                    return true;
                }),
            m_children.end());
    }

    // Recursively update children
    for (auto& child : children) {
        child->update_propagated_sinks(visited);
    }
}

} // namespace SlimLog
