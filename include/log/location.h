#pragma once

#if __has_include(<source_location>)
#include <source_location>
#endif

namespace PlainCloud::Log {

#ifdef __cpp_lib_source_location
using Location = std::source_location;
#else
class Location {
public:
    [[nodiscard]] static constexpr auto current(
#ifndef __has_builtin
#define __has_builtin(__x) 0
#endif
#if __has_builtin(__builtin_FILE) and __has_builtin(__builtin_FUNCTION)                            \
        and __has_builtin(__builtin_LINE)                                                          \
    or defined(_MSC_VER) and _MSC_VER > 192
        // NOLINTNEXTLINE(*-easily-swappable-parameters)
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        int line = __builtin_LINE()
#else
        // NOLINTNEXTLINE(*-easily-swappable-parameters)
        const char* file = "unknown",
        const char* function = "unknown",
        int line = {}
#endif
            ) noexcept
    {
        Location loc{};
        loc.m_file = file;
        loc.m_function = function;
        loc.m_line = line;
        return loc;
    }
    [[nodiscard]] constexpr auto file_name() const noexcept
    {
        return m_file;
    }
    [[nodiscard]] constexpr auto function_name() const noexcept
    {
        return m_function;
    }
    [[nodiscard]] constexpr auto line() const noexcept
    {
        return m_line;
    }

private:
    const char* m_file{""};
    const char* m_function{""};
    int m_line{};
};
#endif
} // namespace PlainCloud::Log
