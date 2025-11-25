/**
 * @file record.h
 * @brief Contains the declaration of the Record and RecordStringView classes.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <version> // IWYU pragma: keep

namespace SlimLog {

/**
 * @brief Logging level enumeration.
 *
 * Specifies the severity of log events.
 */
enum class Level : std::uint8_t {
    Fatal, ///< Very severe error events leading to application abort.
    Error, ///< Error events that might still allow continuation.
    Warning, ///< Potentially harmful situations.
    Info, ///< Informational messages about application progress.
    Debug, ///< Detailed debug information.
    Trace ///< Trace messages for method entry and exit.
};

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
    constexpr RecordStringView(std::basic_string_view<T> str_view) noexcept
        : std::basic_string_view<T>(std::move(str_view))
    {
    }

    /**
     * @brief Constructor from `std::basic_string`.
     * @param str The std::basic_string to construct from.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    constexpr RecordStringView(const std::basic_string<T>& str) noexcept
        : std::basic_string_view<T>(str)
    {
    }

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
 * @brief Represents a log record containing message details.
 *
 * @tparam String String type for storing the message.
 * @tparam Char Character type for the message.
 */
template<typename String, typename Char>
struct Record {
    /** @brief String reference type. */
    using StringRefType = std::reference_wrapper<const String>;
    /** @brief String view type. */
    using StringViewType = RecordStringView<Char>;

    Level level = {}; ///< Log level.
    RecordStringView<char> filename = {}; ///< File name.
    RecordStringView<char> function = {}; ///< Function name.
    std::size_t line = {}; ///< Line number.
    StringViewType category = {}; ///< Log category.
    std::size_t thread_id = {}; ///< Thread ID.
    std::pair<std::chrono::sys_seconds, std::size_t> time; ///< Record time.
    std::variant<StringRefType, StringViewType> message = StringViewType{}; ///< Log message.
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/record-inl.h" // IWYU pragma: keep
#endif
