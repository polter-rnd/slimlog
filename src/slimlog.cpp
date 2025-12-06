#include "slimlog/format.h"
#include "slimlog/logger.h"
#include "slimlog/pattern.h"
#include "slimlog/sinks/callback_sink.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"

#include <slimlog_export.h>

#ifndef SLIMLOG_HEADER_ONLY
// IWYU pragma: begin_keep
#include "slimlog/format-inl.h"
#include "slimlog/logger-inl.h"
#include "slimlog/pattern-inl.h"
#include "slimlog/record-inl.h"
#include "slimlog/sink-inl.h"
#include "slimlog/sinks/callback_sink-inl.h"
#include "slimlog/sinks/file_sink-inl.h"
#include "slimlog/sinks/ostream_sink-inl.h"
// IWYU pragma: end_keep
#endif

#include <chrono>
#include <cstddef>

namespace SlimLog {
// char
template class SLIMLOG_EXPORT Logger<char, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT Logger<char, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT FormattableSink<char>;
template class SLIMLOG_EXPORT CallbackSink<char>;
template class SLIMLOG_EXPORT FileSink<char>;
template class SLIMLOG_EXPORT OStreamSink<char>;
template class SLIMLOG_EXPORT SLIMLOG_EXPORT NullSink<char>;
template class RecordStringView<char>;
template class SLIMLOG_EXPORT Pattern<char>;
template SLIMLOG_EXPORT void Pattern<char>::format(
    Util::MemoryBuffer<char, DefaultBufferSize>&, const Record<char>&);
template class CachedFormatter<std::size_t, char>;
template class CachedFormatter<std::chrono::sys_seconds, char>;
#ifndef SLIMLOG_FMTLIB
template class SLIMLOG_EXPORT FormatValue<std::size_t, char>;
template class SLIMLOG_EXPORT FormatValue<std::chrono::sys_seconds, char>;
#endif

// wchar_t
template class SLIMLOG_EXPORT Logger<wchar_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT Logger<wchar_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT FormattableSink<wchar_t>;
template class SLIMLOG_EXPORT CallbackSink<wchar_t>;
template class SLIMLOG_EXPORT FileSink<wchar_t>;
template class SLIMLOG_EXPORT OStreamSink<wchar_t>;
template class SLIMLOG_EXPORT NullSink<wchar_t>;
template class SLIMLOG_EXPORT RecordStringView<wchar_t>;
template class SLIMLOG_EXPORT Pattern<wchar_t>;
template SLIMLOG_EXPORT void Pattern<wchar_t>::format(
    Util::MemoryBuffer<wchar_t, DefaultBufferSize>&, const Record<wchar_t>&);
template class SLIMLOG_EXPORT CachedFormatter<std::size_t, wchar_t>;
template class SLIMLOG_EXPORT CachedFormatter<std::chrono::sys_seconds, wchar_t>;
#ifndef SLIMLOG_FMTLIB
template class SLIMLOG_EXPORT FormatValue<std::size_t, wchar_t>;
template class SLIMLOG_EXPORT FormatValue<std::chrono::sys_seconds, wchar_t>;
#endif

// char8_t
#ifdef SLIMLOG_CHAR8_T
template class SLIMLOG_EXPORT Logger<char8_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT Logger<char8_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT FormattableSink<char8_t>;
template class SLIMLOG_EXPORT CallbackSink<char8_t>;
template class SLIMLOG_EXPORT FileSink<char8_t>;
template class SLIMLOG_EXPORT OStreamSink<char8_t>;
template class SLIMLOG_EXPORT NullSink<char8_t>;
template class SLIMLOG_EXPORT RecordStringView<char8_t>;
template class SLIMLOG_EXPORT Pattern<char8_t>;
template SLIMLOG_EXPORT void Pattern<char8_t>::format(
    Util::MemoryBuffer<char8_t, DefaultBufferSize>&, const Record<char8_t>&);
template class SLIMLOG_EXPORT CachedFormatter<std::size_t, char8_t>;
template class SLIMLOG_EXPORT CachedFormatter<std::chrono::sys_seconds, char8_t>;
#endif

// char16_t
#ifdef SLIMLOG_CHAR16_T
template class SLIMLOG_EXPORT Logger<char16_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT Logger<char16_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT FormattableSink<char16_t>;
template class SLIMLOG_EXPORT CallbackSink<char16_t>;
template class SLIMLOG_EXPORT FileSink<char16_t>;
template class SLIMLOG_EXPORT OStreamSink<char16_t>;
template class SLIMLOG_EXPORT NullSink<char16_t>;
template class SLIMLOG_EXPORT RecordStringView<char16_t>;
template class SLIMLOG_EXPORT Pattern<char16_t>;
template SLIMLOG_EXPORT void Pattern<char16_t>::format(
    Util::MemoryBuffer<char16_t, DefaultBufferSize>&, const Record<char16_t>&);
template class SLIMLOG_EXPORT CachedFormatter<std::size_t, char16_t>;
template class SLIMLOG_EXPORT CachedFormatter<std::chrono::sys_seconds, char16_t>;
#endif

// char32_t
#ifdef SLIMLOG_CHAR32_T
template class SLIMLOG_EXPORT Logger<char32_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT Logger<char32_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT FormattableSink<char32_t>;
template class SLIMLOG_EXPORT CallbackSink<char32_t>;
template class SLIMLOG_EXPORT FileSink<char32_t>;
template class SLIMLOG_EXPORT OStreamSink<char32_t>;
template class SLIMLOG_EXPORT NullSink<char32_t>;
template class SLIMLOG_EXPORT RecordStringView<char32_t>;
template class SLIMLOG_EXPORT Pattern<char32_t>;
template SLIMLOG_EXPORT void Pattern<char32_t>::format(
    Util::MemoryBuffer<char32_t, DefaultBufferSize>&, const Record<char32_t>&);
template class SLIMLOG_EXPORT CachedFormatter<std::size_t, char32_t>;
template class SLIMLOG_EXPORT CachedFormatter<std::chrono::sys_seconds, char32_t>;
#endif
} // namespace SlimLog
