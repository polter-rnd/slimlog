/**
 * @file null_sink-inl.h
 * @brief Contains definition of NullSink class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/sinks/null_sink.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/sinks/null_sink.h" // IWYU pragma: associated

namespace SlimLog {

template<typename Char>
auto NullSink<Char>::message(const RecordType& /*unused*/) -> void
{
}

template<typename Char>
auto NullSink<Char>::flush() -> void
{
}

} // namespace SlimLog
