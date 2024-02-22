#pragma once

#if __has_include(<format>) and defined(__cpp_lib_format)
#include <format> // IWYU pragma: export
#else
#include <fmt/core.h> // IWYU pragma: export
#include <fmt/format.h> // IWYU pragma: export
#include <fmt/xchar.h>
#endif

namespace PlainCloud::Log {
// Forward declaration of namespace to make IWYU happy
}

#include "location.h"

namespace PlainCloud::Log {

#ifdef __cpp_lib_format
using std::format;
template<typename T, typename... Args>
using BasicFormatString = std::basic_format_string<T, Args...>;
#else
using fmt::format;
template<typename T, typename... Args>
using BasicFormatString = fmt::basic_format_string<T, Args...>;
#endif

template<typename CharT, typename... Args>
struct BasicFormat {
public:
    template<typename T>
        requires std::convertible_to<T, BasicFormatString<CharT, Args...>>
    // NOLINTNEXTLINE(hicpp-explicit-conversions)
    consteval BasicFormat(const T& fmt, const Location& loc = Location::current())
        : m_fmt(fmt)
        , m_loc(loc)
    {
    }

    [[nodiscard]] constexpr auto fmt() const noexcept -> const auto&
    {
        return m_fmt;
    }

    [[nodiscard]] constexpr auto loc() const noexcept -> const auto&
    {
        return m_loc;
    }

private:
    BasicFormatString<CharT, Args...> m_fmt;
    Location m_loc;
};

template<typename... Args>
using Format = BasicFormat<char, std::type_identity_t<Args>...>;

template<typename... Args>
using WideFormat = BasicFormat<wchar_t, std::type_identity_t<Args>...>;

} // namespace PlainCloud::Log
