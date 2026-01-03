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
#include "slimlog/sink-inl.h"
#include "slimlog/sinks/callback_sink-inl.h"
#include "slimlog/sinks/file_sink-inl.h"
#include "slimlog/sinks/ostream_sink-inl.h"
// IWYU pragma: end_keep
#endif

#include <chrono>
#include <cstddef>

#if defined(_WIN32) && (!defined(__clang__) || defined(_MSC_VER))
// On Windows with MSVC or Clang-cl, no need for explicit export of template classes
#define SLIMLOG_EXPORT_CLASS
#else
#define SLIMLOG_EXPORT_CLASS SLIMLOG_EXPORT
#endif

namespace SlimLog {
// char
// On Linux/Unix, or Windows with vanilla Clang: need explicit export due to hidden visibility
template class SLIMLOG_EXPORT_CLASS Logger<char, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS Logger<char, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char, MultiThreadedPolicy>;
template class CallbackSink<char>;
template class NullSink<char>;
template class Pattern<char>;
template SLIMLOG_EXPORT void Pattern<char>::format<SingleThreadedPolicy>(
    FormatBuffer<char, DefaultSinkBufferSize>&, const Record<char>&);
template SLIMLOG_EXPORT void Pattern<char>::format<MultiThreadedPolicy>(
    FormatBuffer<char, DefaultSinkBufferSize>&, const Record<char>&);
template class CachedFormatter<std::size_t, char>;
template class CachedFormatter<std::chrono::sys_seconds, char>;
#ifndef SLIMLOG_FMTLIB
template class FormatValue<std::size_t, char>;
template class FormatValue<std::chrono::sys_seconds, char>;
#endif

// wchar_t
template class SLIMLOG_EXPORT_CLASS Logger<wchar_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS Logger<wchar_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<wchar_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<wchar_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<wchar_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<wchar_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<wchar_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<wchar_t, MultiThreadedPolicy>;
template class CallbackSink<wchar_t>;
template class NullSink<wchar_t>;
template class Pattern<wchar_t>;
template SLIMLOG_EXPORT void Pattern<wchar_t>::format<SingleThreadedPolicy>(
    FormatBuffer<wchar_t, DefaultSinkBufferSize>&, const Record<wchar_t>&);
template SLIMLOG_EXPORT void Pattern<wchar_t>::format<MultiThreadedPolicy>(
    FormatBuffer<wchar_t, DefaultSinkBufferSize>&, const Record<wchar_t>&);
template class CachedFormatter<std::size_t, wchar_t>;
template class CachedFormatter<std::chrono::sys_seconds, wchar_t>;
#ifndef SLIMLOG_FMTLIB
template class FormatValue<std::size_t, wchar_t>;
template class FormatValue<std::chrono::sys_seconds, wchar_t>;
#endif

// char8_t
#ifdef SLIMLOG_CHAR8_T
template class SLIMLOG_EXPORT_CLASS Logger<char8_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS Logger<char8_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char8_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char8_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char8_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char8_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char8_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char8_t, MultiThreadedPolicy>;
template class CallbackSink<char8_t>;
template class NullSink<char8_t>;
template class Pattern<char8_t>;
template SLIMLOG_EXPORT void Pattern<char8_t>::format<SingleThreadedPolicy>(
    FormatBuffer<char8_t, DefaultBufferSize>&, const Record<char8_t>&);
template SLIMLOG_EXPORT void Pattern<char8_t>::format<MultiThreadedPolicy>(
    FormatBuffer<char8_t, DefaultBufferSize>&, const Record<char8_t>&);
template class CachedFormatter<std::size_t, char8_t>;
template class CachedFormatter<std::chrono::sys_seconds, char8_t>;
#endif

// char16_t
#ifdef SLIMLOG_CHAR16_T
template class SLIMLOG_EXPORT_CLASS Logger<char16_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS Logger<char16_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char16_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char16_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char16_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char16_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char16_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char16_t, MultiThreadedPolicy>;
template class CallbackSink<char16_t>;
template class NullSink<char16_t>;
template class Pattern<char16_t>;
template SLIMLOG_EXPORT void Pattern<char16_t>::format<SingleThreadedPolicy>(
    FormatBuffer<char16_t, DefaultBufferSize>&, const Record<char16_t>&);
template SLIMLOG_EXPORT void Pattern<char16_t>::format<MultiThreadedPolicy>(
    FormatBuffer<char16_t, DefaultBufferSize>&, const Record<char16_t>&);
template class CachedFormatter<std::size_t, char16_t>;
template class CachedFormatter<std::chrono::sys_seconds, char16_t>;
#endif

// char32_t
#ifdef SLIMLOG_CHAR32_T
template class SLIMLOG_EXPORT_CLASS Logger<char32_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS Logger<char32_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char32_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FormattableSink<char32_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char32_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS FileSink<char32_t, MultiThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char32_t, SingleThreadedPolicy>;
template class SLIMLOG_EXPORT_CLASS OStreamSink<char32_t, MultiThreadedPolicy>;
template class CallbackSink<char32_t>;
template class NullSink<char32_t>;
template class Pattern<char32_t>;
template SLIMLOG_EXPORT void Pattern<char32_t>::format<SingleThreadedPolicy>(
    FormatBuffer<char32_t, DefaultBufferSize>&, const Record<char32_t>&);
template SLIMLOG_EXPORT void Pattern<char32_t>::format<MultiThreadedPolicy>(
    FormatBuffer<char32_t, DefaultBufferSize>&, const Record<char32_t>&);
template class CachedFormatter<std::size_t, char32_t>;
template class CachedFormatter<std::chrono::sys_seconds, char32_t>;
#endif
} // namespace SlimLog
