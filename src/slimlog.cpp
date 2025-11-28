#include "slimlog/format.h"
#include "slimlog/logger.h"
#include "slimlog/pattern.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"

#ifndef SLIMLOG_HEADER_ONLY
// IWYU pragma: begin_keep
#include "slimlog/format-inl.h"
#include "slimlog/logger-inl.h"
#include "slimlog/pattern-inl.h"
#include "slimlog/record-inl.h"
#include "slimlog/sink-inl.h"
#include "slimlog/sinks/file_sink-inl.h"
#include "slimlog/sinks/null_sink-inl.h"
#include "slimlog/sinks/ostream_sink-inl.h"
// IWYU pragma: end_keep
#endif

#include <chrono>
#include <cstddef>
#include <string>

namespace SlimLog {
// char
template class Logger<std::string, char, SingleThreadedPolicy>;
template class Logger<std::string, char, MultiThreadedPolicy>;
template class FormattableSink<std::string>;
template class FileSink<std::string>;
template class OStreamSink<std::string>;
template class NullSink<std::string>;
template class RecordStringView<char>;
template class Pattern<char>;
template void Pattern<char>::format(
    Util::MemoryBuffer<char, DefaultBufferSize>&, const Record<std::string, char>&);
template class CachedFormatter<std::size_t, char>;
template class CachedFormatter<std::chrono::sys_seconds, char>;
#ifndef SLIMLOG_FMTLIB
template class FormatValue<std::size_t, char>;
template class FormatValue<std::chrono::sys_seconds, char>;
#endif

// wchar_t
template class Logger<std::wstring, wchar_t, SingleThreadedPolicy>;
template class Logger<std::wstring, wchar_t, MultiThreadedPolicy>;
template class FormattableSink<std::wstring>;
template class FileSink<std::wstring>;
template class OStreamSink<std::wstring>;
template class NullSink<std::wstring>;
template class RecordStringView<wchar_t>;
template class Pattern<wchar_t>;
template void Pattern<wchar_t>::format(
    Util::MemoryBuffer<wchar_t, DefaultBufferSize>&, const Record<std::wstring, wchar_t>&);
template class CachedFormatter<std::size_t, wchar_t>;
template class CachedFormatter<std::chrono::sys_seconds, wchar_t>;
#ifndef SLIMLOG_FMTLIB
template class FormatValue<std::size_t, wchar_t>;
template class FormatValue<std::chrono::sys_seconds, wchar_t>;
#endif

// char8_t
#ifdef SLIMLOG_CHAR8_T
template class Logger<std::u8string, char8_t, SingleThreadedPolicy>;
template class Logger<std::u8string, char8_t, MultiThreadedPolicy>;
template class FormattableSink<std::u8string>;
template class FileSink<std::u8string>;
template class OStreamSink<std::u8string>;
template class NullSink<std::u8string>;
template class RecordStringView<char8_t>;
template class Pattern<char8_t>;
template void Pattern<char8_t>::format(
    Util::MemoryBuffer<char8_t, DefaultBufferSize>&, const Record<std::u8string, char8_t>&);
template class CachedFormatter<std::size_t, char8_t>;
template class CachedFormatter<std::chrono::sys_seconds, char8_t>;
#endif

// char16_t
#ifdef SLIMLOG_CHAR16_T
template class Logger<std::u16string, char16_t, SingleThreadedPolicy>;
template class Logger<std::u16string, char16_t, MultiThreadedPolicy>;
template class FormattableSink<std::u16string>;
template class FileSink<std::u16string>;
template class OStreamSink<std::u16string>;
template class NullSink<std::u16string>;
template class RecordStringView<char16_t>;
template class Pattern<char16_t>;
template void Pattern<char16_t>::format(
    Util::MemoryBuffer<char16_t, DefaultBufferSize>&, const Record<std::u16string, char16_t>&);
template class CachedFormatter<std::size_t, char16_t>;
template class CachedFormatter<std::chrono::sys_seconds, char16_t>;
#endif

// char32_t
#ifdef SLIMLOG_CHAR32_T
template class Logger<std::u32string, char32_t, SingleThreadedPolicy>;
template class Logger<std::u32string, char32_t, MultiThreadedPolicy>;
template class FormattableSink<std::u32string>;
template class FileSink<std::u32string>;
template class OStreamSink<std::u32string>;
template class NullSink<std::u32string>;
template class RecordStringView<char32_t>;
template class Pattern<char32_t>;
template void Pattern<char32_t>::format(
    Util::MemoryBuffer<char32_t, DefaultBufferSize>&, const Record<std::u32string, char32_t>&);
template class CachedFormatter<std::size_t, char32_t>;
template class CachedFormatter<std::chrono::sys_seconds, char32_t>;
#endif
} // namespace SlimLog
