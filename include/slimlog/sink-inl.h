/**
 * @file sink-inl.h
 * @brief Contains the definition of Sink and SinkDriver classes.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sink.h" // IWYU pragma: associated

#include <algorithm>
#include <iterator>

namespace SlimLog {

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<String, Char, BufferSize, Allocator>::set_levels(
    std::initializer_list<std::pair<Level, StringViewType>> levels) -> void
{
    m_pattern.set_levels(std::move(levels));
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<String, Char, BufferSize, Allocator>::set_pattern(StringViewType pattern)
    -> void
{
    m_pattern.set_pattern(std::move(pattern));
}

template<typename String, typename Char, std::size_t BufferSize, typename Allocator>
auto FormattableSink<String, Char, BufferSize, Allocator>::format(
    FormatBufferType& result, RecordType& record) -> void
{
    m_pattern.format(result, record);
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::SinkDriver(
    const LoggerType* logger, SinkDriver* parent)
    : m_logger(logger)
    , m_parent(parent)
{
    if (m_parent) {
        m_parent->add_child(this);
        update_effective_sinks();
    }
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::~SinkDriver()
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::add_sink(
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::remove_sink(
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::set_sink_enabled(
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::sink_enabled(
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::parent() -> SinkDriver*
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::set_parent(
    SinkDriver* parent) -> void
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::add_child(SinkDriver* child)
    -> void
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::remove_child(
    SinkDriver* child) -> void
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
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::update_effective_sinks(
    SinkDriver* driver) -> SinkDriver*
{
    typename ThreadingPolicy::ReadLock parent_lock;
    SinkDriver* parent = driver->m_parent;
    if (parent) {
        if (parent != this) {
            // Avoid deadlock when locking parent which is already write-locked
            parent_lock = typename ThreadingPolicy::ReadLock(parent->m_mutex);
        }
        driver->m_effective_sinks = parent->m_effective_sinks;
    } else {
        driver->m_effective_sinks.clear();
    }

    // Update the current node's effective sinks
    for (const auto& [sink, enabled] : driver->m_sinks) {
        if (enabled) {
            driver->m_effective_sinks.insert_or_assign(sink.get(), driver->m_logger);
        } else {
            driver->m_effective_sinks.erase(sink.get());
        }
    }

    // Find the next node in level order
    SinkDriver* next = nullptr;
    if (driver->m_children.empty()) {
        // Move to the next sibling or parent's sibling
        SinkDriver* prev = driver;
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
        next = driver->m_children.front();
    }

    return next;
}

template<
    typename String,
    typename Char,
    typename ThreadingPolicy,
    std::size_t BufferSize,
    typename Allocator>
auto SinkDriver<String, Char, ThreadingPolicy, BufferSize, Allocator>::update_effective_sinks()
    -> void
{
    // Update current driver without lock since update_effective_sinks()
    // have to be called already under the write lock.
    SinkDriver* next = update_effective_sinks(this);
    while (next) {
        const typename ThreadingPolicy::WriteLock next_lock(next->m_mutex);
        next = update_effective_sinks(next);
    }
}

} // namespace SlimLog
