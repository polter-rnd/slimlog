/**
 * @file ostream_sink-inl.h
 * @brief Contains definition of OStreamSink class.
 */

#pragma once

// IWYU pragma: private, include <slimlog/sinks/ostream_sink.h>

#ifndef SLIMLOG_HEADER_ONLY
#include <slimlog/sinks/ostream_sink.h>
#endif

#include <slimlog/logger.h>
#include <slimlog/sink.h>

#include <ostream>

namespace SlimLog {

template<typename Logger>
auto OStreamSink<Logger>::message(RecordType& record) -> void
{
    FormatBufferType buffer;
    Sink<Logger>::format(buffer, record);
    buffer.push_back('\n');
    m_ostream.write(buffer.begin(), buffer.size());
}

template<typename Logger>
auto OStreamSink<Logger>::flush() -> void
{
    m_ostream.flush();
}

} // namespace SlimLog
