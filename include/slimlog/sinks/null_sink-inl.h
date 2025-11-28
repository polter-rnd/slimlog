/**
 * @file null_sink-inl.h
 * @brief Contains definition of NullSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/null_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/null_sink.h" // IWYU pragma: associated

namespace SlimLog {

template<typename String, typename Char>
auto NullSink<String, Char>::message(const RecordType& /*unused*/) -> void
{
}

template<typename String, typename Char>
auto NullSink<String, Char>::flush() -> void
{
}

} // namespace SlimLog
