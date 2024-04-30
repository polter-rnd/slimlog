
#pragma once

#include "level.h"
#include "location.h"
#include "policy.h"

#include <atomic>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <utility>

namespace PlainCloud::Log {

template<typename StringT>
class Sink {
public:
    Sink() = default;
    Sink(Sink const&) = default;
    Sink(Sink&&) noexcept = default;
    auto operator=(Sink const&) -> Sink& = default;
    auto operator=(Sink&&) noexcept -> Sink& = default;
    virtual ~Sink() = default;

    virtual auto emit(Level level, const StringT& message, const Location& location) const -> void
        = 0;
    virtual auto flush() -> void = 0;
};

template<typename StringT, typename ThreadingPolicy>
class SinkDriver { };

template<typename StringT>
class SinkDriver<StringT, SingleThreadedPolicy> {
public:
    SinkDriver(const std::initializer_list<std::shared_ptr<Sink<StringT>>>& sinks = {}) noexcept
    {
        m_sinks.reserve(sinks.size());
        for (const auto& sink : sinks) {
            m_sinks.emplace(sink, true);
        }
    }

    auto add_sink(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        return m_sinks.insert_or_assign(sink, true).second;
    }

    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<StringT>>
    {
        return m_sinks.insert_or_assign(std::make_shared<T>(std::forward<Args>(args)...), true)
            .first->first;
    }

    auto remove_sink(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        return m_sinks.erase(sink) == 1;
    }

    auto set_sink_enabled(const std::shared_ptr<Sink<StringT>>& sink, bool enabled) -> bool
    {
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            itr->second = enabled;
            return true;
        }
        return false;
    }

    auto sink_enabled(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        if (const auto itr = m_sinks.find(sink); itr != m_sinks.end()) {
            return itr->second;
        }
        return false;
    }

    template<typename LoggerT, typename T, typename... Args>
        requires std::invocable<T, Args...>
    auto emit(
        const LoggerT& logger,
        const Log::Level level,
        const T& callback,
        const Log::Location& location = Log::Location::current(),
        Args&&... args) const -> void
    {
        if (static_cast<Level>(logger.m_level) < level) {
            return;
        }

        auto sinks{m_sinks};
        for (auto parent{logger.m_parent}; parent; parent = parent->m_parent) {
            if (static_cast<Level>(parent->m_level) >= level) {
                sinks.insert(parent->m_sinks.m_sinks.cbegin(), parent->m_sinks.m_sinks.cend());
            }
        }
        for (const auto& sink : sinks) {
            if (sink.second) {
                // NOLINTNEXTLINE(*-array-to-pointer-decay,*-no-array-decay)
                sink.first->emit(level, callback(std::forward<Args>(args)...), location);
            }
        }
    }

private:
    std::unordered_map<std::shared_ptr<Sink<StringT>>, bool> m_sinks;
};

template<
    typename StringT,
    typename MutexT,
    typename ReadLockT,
    typename WriteLockT,
    std::memory_order LoadOrder,
    std::memory_order StoreOrder>
class SinkDriver<StringT, MultiThreadedPolicy<MutexT, ReadLockT, WriteLockT, LoadOrder, StoreOrder>>
    : public SinkDriver<StringT, SingleThreadedPolicy> {
public:
    using SinkDriver<StringT, SingleThreadedPolicy>::SinkDriver;

    auto add_sink(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        WriteLockT lock(m_mutex);
        return SinkDriver<StringT, SingleThreadedPolicy>::add_sink(sink);
    }

    template<typename T, typename... Args>
    auto add_sink(Args&&... args) -> std::shared_ptr<Sink<StringT>>
    {
        WriteLockT lock(m_mutex);
        return SinkDriver<StringT, SingleThreadedPolicy>::template add_sink<T>(
            std::forward<Args>(args)...);
    }

    auto remove_sink(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        WriteLockT lock(m_mutex);
        return SinkDriver<StringT, SingleThreadedPolicy>::remove_sink(sink);
    }

    auto set_sink_enabled(const std::shared_ptr<Sink<StringT>>& sink, bool enabled) -> bool
    {
        WriteLockT lock(m_mutex);
        return SinkDriver<StringT, SingleThreadedPolicy>::set_sink_enabled(sink, enabled);
    }

    auto sink_enabled(const std::shared_ptr<Sink<StringT>>& sink) -> bool
    {
        ReadLockT lock(m_mutex);
        return SinkDriver<StringT, SingleThreadedPolicy>::sink_enabled(sink);
    }

    template<typename LoggerT, typename T, typename... Args>
        requires std::invocable<T, Args...>
    auto emit(
        const LoggerT& logger,
        const Log::Level level,
        const T& callback,
        const Log::Location& location = Log::Location::current(),
        Args&&... args) const -> void
    {
        ReadLockT lock(m_mutex);
        SinkDriver<StringT, SingleThreadedPolicy>::emit(
            logger, level, callback, location, std::forward<Args>(args)...);
    }

private:
    mutable MutexT m_mutex;
};

} // namespace PlainCloud::Log
