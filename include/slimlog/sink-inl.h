/**
 * @file sink-inl.h
 * @brief Contains the definition of Sink and SinkDriver classes.
 */

#pragma once

// IWYU pragma: private, include <slimlog/sink.h>

#ifndef SLIMLOG_HEADER_ONLY
#include <slimlog/logger.h>
#include <slimlog/sink.h>
#endif

#include <slimlog/level.h>
#include <slimlog/location.h>
#include <slimlog/pattern.h>
#include <slimlog/policy.h>
#include <slimlog/record.h>
#include <slimlog/util/os.h>

#include <algorithm> // IWYU pragma: keep
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace SlimLog {

template<typename Logger>
auto Sink<Logger>::set_pattern(StringViewType pattern) -> std::shared_ptr<Sink<Logger>>
{
    m_pattern.set_pattern(std::move(pattern));
    return this->shared_from_this();
}

template<typename Logger>
auto Sink<Logger>::set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels)
    -> std::shared_ptr<Sink<Logger>>
{
    m_pattern.set_levels(std::move(levels));
    return this->shared_from_this();
}

template<typename Logger>
auto Sink<Logger>::format(FormatBufferType& result, RecordType& record) -> void
{
    m_pattern.format(result, record);
}

template<typename Logger, typename ThreadingPolicy>
SinkDriver<Logger, ThreadingPolicy>::SinkDriver(const Logger* logger, SinkDriver* parent)
    : m_logger(logger)
    , m_parent(parent)
{
    if (m_parent) {
        m_parent->add_child(this);
        update_effective_sinks();
    }
}

template<typename Logger, typename ThreadingPolicy>
SinkDriver<Logger, ThreadingPolicy>::~SinkDriver()
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    for (auto* child : m_children) {
        child->set_parent(m_parent);
    }
    if (m_parent) {
        m_parent->remove_child(this);
    }
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::add_sink(const std::shared_ptr<Sink<Logger>>& sink)
    -> bool
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    const auto result = m_sinks.insert_or_assign(sink, true).second;
    update_effective_sinks();
    return result;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::remove_sink(const std::shared_ptr<Sink<Logger>>& sink)
    -> bool
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    if (m_sinks.erase(sink) == 1) {
        update_effective_sinks();
        return true;
    }
    return false;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::set_sink_enabled(
    const std::shared_ptr<Sink<Logger>>& sink, bool enabled) -> bool
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
        itr->second = enabled;
        update_effective_sinks();
        return true;
    }
    return false;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::sink_enabled(const std::shared_ptr<Sink<Logger>>& sink)
    -> bool
{
    const typename ThreadingPolicy::ReadLock lock(m_mutex);
    if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
        return itr->second;
    }
    return false;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::parent() -> SinkDriver*
{
    const typename ThreadingPolicy::ReadLock lock(m_mutex);
    return m_parent;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::set_parent(SinkDriver* parent) -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_parent = parent;
    update_effective_sinks();
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::add_child(SinkDriver* child) -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_children.insert(child);
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::remove_child(SinkDriver* child) -> void
{
    const typename ThreadingPolicy::WriteLock lock(m_mutex);
    m_children.erase(child);
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::create_record(
    Level level, StringViewType category, Location location) -> RecordType
{
    RecordType record = {
        level,
        {location.file_name(), location.function_name(), static_cast<std::size_t>(location.line())},
        std::move(category),
        Util::OS::thread_id()};
    std::tie(record.time.local, record.time.nsec) = Util::OS::local_time();
    return record;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::update_effective_sinks(SinkDriver* driver) -> SinkDriver*
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
        while (parent && !next) {
            if (auto prev_it = parent->m_children.find(prev);
                std::distance(prev_it, parent->m_children.end()) > 1) {
                next = *std::next(prev_it);
            }
            prev = parent;
            parent = parent->m_parent != this ? parent->m_parent : nullptr;
        }
    } else {
        // Nove to next level
        next = *driver->m_children.begin();
    }

    return next;
}

template<typename Logger, typename ThreadingPolicy>
auto SinkDriver<Logger, ThreadingPolicy>::update_effective_sinks() -> void
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
