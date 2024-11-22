/**
 * @file sink-int.h
 * @brief Contains the definition of Sink and SinkDriver classes.
 */

#pragma once

// IWYU pragma: private, include <slimlog/sink.h>

#ifndef SLIMLOG_HEADER_ONLY
#include <slimlog/logger.h>
#include <slimlog/sink.h>
#endif

#include <slimlog/level.h>
#include <slimlog/pattern.h>
#include <slimlog/policy.h>

#include <algorithm> // IWYU pragma: keep
#include <exception>
#include <initializer_list>
#include <memory>
#include <queue>
#include <string_view>
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
    try {
        const typename ThreadingPolicy::WriteLock lock(m_mutex);
        for (auto* child : m_children) {
            child->set_parent(m_parent);
        }
        if (m_parent) {
            m_parent->remove_child(this);
        }
    } catch (...) {
        std::terminate();
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
auto SinkDriver<Logger, ThreadingPolicy>::update_effective_sinks() -> void
{
    // Queue for level order traversal
    std::queue<SinkDriver*> nodes;
    nodes.push(this);
    while (!nodes.empty()) {
        auto* node = nodes.front();
        if (node->m_parent) {
            node->m_effective_sinks = node->m_parent->m_effective_sinks;
            for (const auto& [sink, enabled] : node->m_sinks) {
                if (enabled) {
                    node->m_effective_sinks.insert_or_assign(sink.get(), node->m_logger);
                } else {
                    node->m_effective_sinks.erase(sink.get());
                }
            }
        } else {
            node->m_effective_sinks.clear();
            for (const auto& [sink, enabled] : node->m_sinks) {
                if (enabled) {
                    node->m_effective_sinks.emplace(sink.get(), node->m_logger);
                }
            }
        }
        nodes.pop();

        for (auto* child : node->m_children) {
            nodes.push(child);
        }
    }
}

} // namespace SlimLog
