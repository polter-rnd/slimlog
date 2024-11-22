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
// char
template class SinkDriver<Logger<std::string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::string_view>, MultiThreadedPolicy>;
template class Sink<Logger<std::string_view>>;
template class RecordStringView<char>;
template class Pattern<char>;

// wchar_t
template class SinkDriver<Logger<std::wstring_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::wstring_view>, MultiThreadedPolicy>;
template class Sink<Logger<std::wstring_view>>;
template class RecordStringView<wchar_t>;
template class Pattern<wchar_t>;

// char8_t (std::mbrtoc8() is supported only for newer versions of glibc++ and libc++)
/*#if defined(__cpp_char8_t)
#if defined(_GLIBCXX_USE_UCHAR_C8RTOMB_MBRTOC8_CXX20)                                              \
    or defined(_LIBCPP_VERSION) and !defined(_LIBCPP_HAS_NO_C8RTOMB_MBRTOC8)
template class SinkDriver<Logger<std::u8string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::u8string_view>, MultiThreadedPolicy>;
template class Sink<Logger<std::u8string_view>>;
template class RecordStringView<char8_t>;
template class Pattern<char8_t>;
#endif
#endif

// char16_t
#if defined(__cpp_unicode_characters)
template class SinkDriver<Logger<std::u16string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::u16string_view>, MultiThreadedPolicy>;
template class Sink<Logger<std::u16string_view>>;
template class RecordStringView<char16_t>;
template class Pattern<char16_t>;

// char32_t
template class SinkDriver<Logger<std::u32string_view>, SingleThreadedPolicy>;
template class SinkDriver<Logger<std::u32string_view>, MultiThreadedPolicy>;
template class Sink<Logger<std::u32string_view>>;
template class RecordStringView<char32_t>;
template class Pattern<char32_t>;
#endif*/
} // namespace SlimLog
