/**
 * @file null_sink-inl.h
 * @brief Contains definition of NullSink class.
 */

#pragma once

// IWYU pragma: private, include <slimlog/sinks/null_sink.h>

#ifndef SLIMLOG_HEADER_ONLY
#include <slimlog/sinks/null_sink.h>
#endif

#include <slimlog/logger.h>
#include <slimlog/sink.h>

namespace SlimLog {

template<typename Logger>
auto NullSink<Logger>::message(RecordType& record) -> void
{
    FormatBufferType buffer;
    Sink<Logger>::format(buffer, record);
    buffer.push_back('\n');
}

template<typename Logger>
auto NullSink<Logger>::flush() -> void
{
}

} // namespace SlimLog
