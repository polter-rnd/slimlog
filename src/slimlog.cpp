#include <slimlog/logger.h>
#include <slimlog/pattern-inl.h> // IWYU pragma: keep
#include <slimlog/pattern.h>
#include <slimlog/policy.h>
#include <slimlog/sink-inl.h> // IWYU pragma: keep
#include <slimlog/sink.h>

#include <string_view>

namespace SlimLog {
template class SinkDriver<Logger<std::string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::string_view>, MultiThreadedPolicy>;

template class Sink<Logger<std::string_view>>;

template class Pattern<char>;
} // namespace SlimLog