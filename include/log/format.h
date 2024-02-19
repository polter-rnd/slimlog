#pragma once

#if __has_include(<format>) and defined(__cpp_lib_format)
#include <format>
#else
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/xchar.h>
#endif

#include "location.h"

namespace plaincloud::Log {

namespace Fmt {
#ifdef __cpp_lib_format
using std::format;
using std::format_string;
using std::wformat_string;
#else
using fmt::format;
using fmt::format_string;
using fmt::wformat_string;
#endif
} // namespace Fmt

template<typename Fmt, typename... Args>
struct BasicFormat {
    template<typename T>
        requires std::convertible_to<T, Fmt>
    consteval BasicFormat(const T& fmt, const Location& loc = Location::current())
        : m_fmt(fmt)
        , m_loc(loc)
    {
    }

    [[nodiscard]] constexpr auto fmt() const noexcept -> const Fmt&
    {
        return m_fmt;
    }

    [[nodiscard]] constexpr auto loc() const noexcept -> const Location&
    {
        return m_loc;
    }

private:
    Fmt m_fmt;
    Location m_loc;
};

template<typename... Args>
using Format = BasicFormat<Fmt::format_string<std::type_identity_t<Args>...>>;

template<typename... Args>
using WideFormat = BasicFormat<Fmt::wformat_string<std::type_identity_t<Args>...>>;

} // namespace plaincloud::Log
