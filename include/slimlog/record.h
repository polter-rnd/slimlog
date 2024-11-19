/**
 * @file record.h
 * @brief Contains the declaration of the Record and RecordStringView classes.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string_view>
#include <variant>

#ifdef SLIMLOG_FMTLIB
// IWYU pragma: no_forward_declare tm
#include <ctime> // IWYU pragma: export
#else
#include <chrono> // IWYU pragma: export
#include <cstddef> // IWYU pragma: export
#endif

namespace SlimLog {

enum class Level : std::uint8_t;

/**
 * @brief Record string view type.
 *
 * This is similar to `std::basic_string_view<T>` but includes a `codepoints()` method
 * to calculate the number of symbols in a Unicode string.
 *
 * @tparam T Character type.
 */
template<typename T>
class RecordStringView : public std::basic_string_view<T> {
public:
    using std::basic_string_view<T>::basic_string_view;

    ~RecordStringView() = default;

    /**
     * @brief Copy constructor.
     * @param str_view The RecordStringView to copy from.
     */
    RecordStringView(const RecordStringView& str_view) noexcept;

    /**
     * @brief Move constructor.
     * @param str_view The RecordStringView to move from.
     */
    RecordStringView(RecordStringView&& str_view) noexcept;

    /**
     * @brief Constructor from `std::basic_string_view`.
     * @param str_view The std::basic_string_view to construct from.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    RecordStringView(std::basic_string_view<T> str_view) noexcept;

    /**
     * @brief Assignment operator.
     * @param str_view The RecordStringView to assign from.
     * @return Reference to this RecordStringView.
     */
    auto operator=(const RecordStringView& str_view) noexcept -> RecordStringView&;

    /**
     * @brief Move assignment operator.
     * @param str_view The RecordStringView to move from.
     * @return Reference to this RecordStringView.
     */
    auto operator=(RecordStringView&& str_view) noexcept -> RecordStringView&;

    /**
     * @brief Assignment from `std::basic_string_view`.
     * @param str_view The std::basic_string_view to assign from.
     * @return Reference to this RecordStringView.
     */
    auto operator=(std::basic_string_view<T> str_view) noexcept -> RecordStringView&;

    /**
     * @brief Calculate the number of Unicode code points.
     *
     * @return Number of code points.
     */
    auto codepoints() -> std::size_t;

private:
    std::atomic<std::size_t> m_codepoints = std::string_view::npos;
};

/**
 * @brief Deduction guide for a constructor call with a pointer and size.
 *
 * @tparam Char Character type for the string view.
 */
template<typename Char>
RecordStringView(const Char*, std::size_t) -> RecordStringView<Char>;

/**
 * @brief Time tag of the log record.
 */
struct RecordTime {
#ifdef SLIMLOG_FMTLIB
    /**
     * @brief Alias for \a std::tm.
     *
     * Time format with `fmt::format()` is much faster for std::tm.
     */
    using TimePoint = std::tm;
#else
    /**
     * @brief Alias for \a std::chrono::sys_seconds.
     *
     * Time format with `std::format()` supports only `std::chrono` types.
     */
    using TimePoint = std::chrono::sys_seconds;
#endif
    TimePoint local; ///< Local time (seconds precision).
    std::size_t nsec; ///< Event time (nanoseconds part).
};

/**
 * @brief Source code location.
 */
struct RecordLocation {
    RecordStringView<char> filename = {}; ///< File name.
    RecordStringView<char> function = {}; ///< Function name.
    std::size_t line = {}; ///< Line number.
};

/**
 * @brief Represents a log record containing message details.
 *
 * @tparam Char Character type for the message.
 * @tparam String String type for storing the message.
 */
template<typename Char, typename String>
struct Record {
    Level level = {}; ///< Log level.
    RecordLocation location = {}; ///< Source code location.
    RecordStringView<Char> category = {}; ///< Log category.
    std::size_t thread_id = {}; ///< Thread ID.
    RecordTime time = {}; ///< Record time.
    std::variant<std::reference_wrapper<String>, RecordStringView<Char>> message
        = RecordStringView<Char>{}; ///< Log message.
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include <slimlog/record-inl.h> // IWYU pragma: keep
#endif
