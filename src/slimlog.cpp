#include <slimlog/logger.h>
#include <slimlog/pattern.h>
#include <slimlog/policy.h>
#include <slimlog/record.h>
#include <slimlog/sink.h>

#ifndef SLIMLOG_HEADER_ONLY
// IWYU pragma: begin_keep
#include <slimlog/pattern-inl.h>
#include <slimlog/record-inl.h>
#include <slimlog/sink-inl.h>
// IWYU pragma: end_keep
#endif

#include <string_view>

namespace SlimLog {
template class SinkDriver<Logger<std::string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::string_view>, MultiThreadedPolicy>;

template class Sink<Logger<std::string_view>>;

template class Pattern<char>;

template class RecordStringView<char>;
} // namespace SlimLog
