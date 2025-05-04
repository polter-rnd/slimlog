/**
 * @file location.h
 * @brief Contains the definition of the Location class.
 */

#pragma once

#if __cpp_lib_source_location
#include <source_location>
#endif

namespace SlimLog {

#ifdef __cpp_lib_source_location
/** @brief Alias for std::source_location. */
using Location = std::source_location;
#else
/**
 * @brief Represents a specific location in the source code.
 *
 * Provides information about the source file, function, and line number.
 */
class Location {
public:
    /**
     * @brief Captures the current source location.
     *
     * @param file The name of the source file.
     * @param function The name of the function.
     * @param line The line number in the source file.
     * @return A Location object representing the current source location.
     */
    [[nodiscard]] static constexpr auto current(
#ifndef __has_builtin
/** @cond */
#define __has_builtin(__x) 0
/** @endcond */
#endif
#if __has_builtin(__builtin_FILE) and __has_builtin(__builtin_FUNCTION)                            \
        and __has_builtin(__builtin_LINE)                                                          \
    or defined(_MSC_VER) and _MSC_VER > 192
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        int line = __builtin_LINE()
#else
        const char* file = "unknown", const char* function = "unknown", int line = -1
#endif
            ) noexcept
    {
        Location loc{};
        loc.m_file = file;
        loc.m_function = function;
        loc.m_line = line;
        return loc;
    }

    /**
     * @brief Gets the source file name.
     *
     * @return The name of the source file.
     */
    [[nodiscard]] constexpr auto file_name() const noexcept
    {
        return m_file;
    }

    /**
     * @brief Gets the function name.
     *
     * @return The name of the function.
     */
    [[nodiscard]] constexpr auto function_name() const noexcept
    {
        return m_function;
    }

    /**
     * @brief Gets the line number.
     *
     * @return The line number in the source file.
     */
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

} // namespace SlimLog
