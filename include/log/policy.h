#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace PlainCloud::Log {

struct SingleThreadedPolicy { };

template<
    typename Mutex = std::shared_mutex,
    typename ReadLock = std::shared_lock<Mutex>,
    typename WriteLock = std::unique_lock<Mutex>,
    std::memory_order LoadOrder = std::memory_order_relaxed,
    std::memory_order StoreOrder = std::memory_order_relaxed>
struct MultiThreadedPolicy { };

} // namespace PlainCloud::Log
