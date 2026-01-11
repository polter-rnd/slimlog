/**
 * @file location.h
 * @brief Contains the definition of the Location class.
 */

#pragma once

namespace slimlog {

/** @cond */
// MSVC supports source_location starting from version 16.6 (MSVC-PR-229114)
#if (!defined(__GNUC__) && !defined(__clang__) && !defined(_MSC_VER))                              \
    or (defined(_MSC_VER) and _MSC_VER < 1926)
#define SLIMLOG_SOURCE_FILE "unknown"
#define SLIMLOG_SOURCE_FUNCTION "unknown"
#define SLIMLOG_SOURCE_LINE 0
#else
#define SLIMLOG_SOURCE_FILE __builtin_FILE()
#define SLIMLOG_SOURCE_FUNCTION __builtin_FUNCTION()
#define SLIMLOG_SOURCE_LINE __builtin_LINE()
#endif

namespace detail {
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
} // namespace detail
/** @endcond */

/**
 * @brief Represents a specific location in the source code.
 *
 * Has the same interface as `std::source_location`.
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
        const char* file = detail::source_basename(),
        const char* function = SLIMLOG_SOURCE_FUNCTION,
        int line = SLIMLOG_SOURCE_LINE) noexcept
    {
        Location loc;
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
    const char* m_file{nullptr};
    const char* m_function{nullptr};
    int m_line{};
};

} // namespace slimlog
