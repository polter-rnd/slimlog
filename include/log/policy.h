#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace PlainCloud::Log {

struct SingleThreadedPolicy { };

template<
    typename MutexT = std::shared_mutex,
    typename ReadLockT = std::shared_lock<MutexT>,
    typename WriteLockT = std::unique_lock<MutexT>,
    std::memory_order LoadOrder = std::memory_order_relaxed,
    std::memory_order StoreOrder = std::memory_order_relaxed>
struct MultiThreadedPolicy { };

} // namespace PlainCloud::Log