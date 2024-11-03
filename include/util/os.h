/**
 * @file os.h
 * @brief Provides OS-specific functions.
 */

#pragma once

#include <chrono>
#include <ctime> // IWYU pragma: no_forward_declare tm
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> // for GetCurrentThreadId
#else
#include <unistd.h>
#ifdef __linux__
#include <sys/syscall.h> // use gettid() syscall under linux to get thread id
#elif defined(_AIX)
#include <pthread.h> // for pthread_getthrds_np
#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <pthread_np.h> // for pthread_getthreadid_np
#elif defined(__NetBSD__)
#include <lwp.h> // for _lwp_self
#elif defined(__sun)
#include <thread.h> // for thr_self
#elif defined(__APPLE__)
#include <AvailabilityMacros.h> // for MAC_OS_X_VERSION_MAX_ALLOWED
#include <pthread.h> // for pthread_threadid_np
#endif
#endif

namespace PlainCloud::Util::OS {

/**
 * @brief Retrieves current thread ID in a cross-platform way.
 *
 * Uses `GetCurrentThreadId()()` on Windows, `gettid` syscall on Linux,
 * `pthread` on other UNIX-based systems.
 * Falls back to `std::this_thread::get_id()` in other cases.
 *
 * @return Current thread ID.
 */
inline auto thread_id() noexcept -> std::size_t
{
    static thread_local std::size_t cached_tid;

    if (cached_tid == 0) {
#ifdef _WIN32
        cached_tid = static_cast<size_t>(::GetCurrentThreadId());
#elif defined(__linux__)
#if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
#define SYS_gettid __NR_gettid
#endif
        // NOLINTNEXTLINE (*-vararg)
        cached_tid = static_cast<size_t>(::syscall(SYS_gettid));
#elif defined(_AIX)
        struct __pthrdsinfo buf;
        int reg_size = 0;
        pthread_t pt = pthread_self();
        int retval
            = pthread_getthrds_np(&pt, PTHRDSINFO_QUERY_TID, &buf, sizeof(buf), NULL, &reg_size);
        int tid = (!retval) ? buf.__pi_tid : 0;
        cached_tid = static_cast<std::size_t>(tid);
#elif defined(__DragonFly__) || defined(__FreeBSD__)
        cached_tid = static_cast<std::size_t>(::pthread_getthreadid_np());
#elif defined(__NetBSD__)
        cached_tid = static_cast<std::size_t>(::_lwp_self());
#elif defined(__OpenBSD__)
        cached_tid = static_cast<std::size_t>(::getthrid());
#elif defined(__sun)
        cached_tid = static_cast<std::size_t>(::thr_self());
#elif defined(__APPLE__)
        uint64_t tid;
// There is no pthread_threadid_np prior to Mac OS X 10.6, and it is not supported on any PPC,
// including 10.6.8 Rosetta. __POWERPC__ is Apple-specific define encompassing ppc and ppc64.
#ifdef MAC_OS_X_VERSION_MAX_ALLOWED
        {
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060) || defined(__POWERPC__)
            tid = pthread_mach_thread_np(pthread_self());
#elif MAC_OS_X_VERSION_MIN_REQUIRED < 1060
            if (&pthread_threadid_np) {
                pthread_threadid_np(nullptr, &tid);
            } else {
                tid = pthread_mach_thread_np(pthread_self());
            }
#else
            pthread_threadid_np(nullptr, &tid);
#endif
        }
#else
        pthread_threadid_np(nullptr, &tid);
#endif
        cached_tid = static_cast<std::size_t>(tid);
#else // Default to standard C++11 (other Unix)
        cached_tid
            = static_cast<std::size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
    }

    return cached_tid;
}

/**
 * @brief Retrieves local time in a cross-platform way.
 *
 * Uses `localtime_s` on Windows and `localtime_r` on other systems.
 *
 * @tparam TimePoint type of resulting time point. Can be `std::chrono::time_point<>` or `std::tm`.
 * @return Pair of local time in seconds and additional nanoseconds.
 */
template<typename TimePoint>
inline auto local_time() noexcept -> std::pair<TimePoint, std::size_t>
{
    static thread_local TimePoint cached_local;
    static thread_local std::time_t cached_time;

    std::timespec curtime{};
#ifdef __linux__
    ::clock_gettime(CLOCK_REALTIME_COARSE, &curtime);
#else
    std::timespec_get(&curtime, TIME_UTC);
#endif

    if (curtime.tv_sec != cached_time) {
        cached_time = curtime.tv_sec;
        if constexpr (std::is_same_v<TimePoint, std::tm>) {
#ifdef _WIN32
            ::localtime_s(&cached_local, &curtime.tv_sec);
#else
            ::localtime_r(&curtime.tv_sec, &cached_local);
#endif
        } else {
            cached_local = TimePoint(std::chrono::duration_cast<typename TimePoint::duration>(
                std::chrono::current_zone()
                    ->to_local(std::chrono::sys_seconds(std::chrono::seconds(curtime.tv_sec)))
                    .time_since_epoch()));
        }
    }

    return std::make_pair(cached_local, static_cast<std::size_t>(curtime.tv_nsec));
}

} // namespace PlainCloud::Util::OS