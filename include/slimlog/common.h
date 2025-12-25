/**
 * @file common.h
 * @brief Contains the declaration of the common enums and structs.
 */

#pragma once

#include "slimlog/threading.h"
#include "slimlog/util/string.h"

#include <cstddef>
#include <cstdint>

namespace SlimLog {

enum : std::uint16_t {
    /** @brief Default buffer size for raw log messages. */
    DefaultBufferSize = 192U,

    /** @brief Default per-sink buffer size for formatted log messages. */
    DefaultSinkBufferSize = 256U
};

/**
 * @brief Default threading policy for logger sinks.
 */
using DefaultThreadingPolicy = SingleThreadedPolicy;

/**
 * @brief Logging level enumeration.
 *
 * Specifies the severity of log events.
 */
enum class Level : std::uint8_t {
    Fatal, ///< Very severe error events leading to application abort.
    Error, ///< Error events that might still allow continuation.
    Warning, ///< Potentially harmful situations.
    Info, ///< Informational messages about application progress.
    Debug, ///< Detailed debug information.
    Trace ///< Trace messages for method entry and exit.
};

/**
 * @brief Represents a log record containing message details.
 *
 * @tparam Char Character type for the message.
 */
template<typename Char>
struct Record {
    CachedStringView<Char> message; ///< Log message.
    CachedStringView<Char> category; ///< Log category.
    CachedStringView<char> filename; ///< File name.
    CachedStringView<char> function; ///< Function name.
    std::size_t line = {}; ///< Line number.
    Level level = {}; ///< Log level.
};

} // namespace SlimLog
