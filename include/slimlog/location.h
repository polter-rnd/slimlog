/**
 * @file location.h
 * @brief Contains the definition of the Location class.
 */

#pragma once

namespace SlimLog {

// MSVC doesn't have __has_builtin, source_location supported from MSVC 16.6 (MSVC-PR-229114)
#if (!defined(__has_builtin) and !defined(_MSC_VER)) or (defined(_MSC_VER) and _MSC_VER < 1926)
#define SLIMLOG_SOURCE_FILE "unknown"
#define SLIMLOG_SOURCE_FUNCTION "unknown"
#define SLIMLOG_SOURCE_LINE -1
#else
#define SLIMLOG_SOURCE_FILE __builtin_FILE()
#define SLIMLOG_SOURCE_FUNCTION __builtin_FUNCTION()
#define SLIMLOG_SOURCE_LINE __builtin_LINE()
#endif

/** @cond */
namespace Detail {
static consteval auto source_basename(const char* path = SLIMLOG_SOURCE_FILE) -> const char*
{
    const char* filename = path;
    while (*path != '\0') {
        if (*path == '\\' || *path == '/') {
            filename = path + 1;
        }
        ++path;
    }
    return filename;
}
} // namespace Detail
/** @endcond */

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
        const char* file = Detail::source_basename(),
        const char* function = SLIMLOG_SOURCE_FUNCTION,
        int line = SLIMLOG_SOURCE_LINE) noexcept
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

} // namespace SlimLog
