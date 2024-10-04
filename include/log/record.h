/**
 * @file record.h
 * @brief Contains definition of Record class.
 */

#pragma once

#include "util/unicode.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <string_view>
#include <variant>

namespace PlainCloud::Log {

enum class Level : std::uint8_t;

/**
 * @brief Record string view type
 *
 * Same as `std::basic_string_view<T>` but with codepoints() method
 * to calculate number of symbols in unicode string.
 *
 * @tparam T Char type
 */
template<typename T>
class RecordStringView : public std::basic_string_view<T> {
public:
    using std::basic_string_view<T>::basic_string_view;

    constexpr ~RecordStringView() = default;

    /**
     * @brief Copy constructor.
     */
    constexpr RecordStringView(const RecordStringView& str_view) noexcept
        : std::basic_string_view<T>(str_view)
        , m_codepoints(str_view.m_codepoints.load(std::memory_order_relaxed))
    {
    }

    /**
     * @brief Move constructor.
     */
    constexpr RecordStringView(RecordStringView&& str_view) noexcept
        : std::basic_string_view<T>(std::move(static_cast<std::basic_string_view<T>&&>(str_view)))
        , m_codepoints(str_view.m_codepoints.load(std::memory_order_relaxed))
    {
    }

    /**
     * @brief Constructor from `std::basic_string_view`.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    constexpr RecordStringView(const std::basic_string_view<T>& str_view) noexcept
        : std::basic_string_view<T>(str_view)
    {
    }

    /**
     * @brief Assignment operator.
     */
    constexpr auto operator=(const RecordStringView& str_view) noexcept -> RecordStringView&
    {
        if (this == &str_view) {
            return *this;
        }

        std::basic_string_view<T>::operator=(str_view);
        m_codepoints.store(
            str_view.m_codepoints.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    /**
     * @brief Move assignment operator.
     */
    constexpr auto operator=(RecordStringView&& str_view) noexcept -> RecordStringView&
    {
        if (this == &str_view) {
            return *this;
        }

        std::basic_string_view<T>::operator=(
            std::move(static_cast<std::basic_string_view<T>&&>(str_view)));
        m_codepoints.store(
            str_view.m_codepoints.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    /**
     * @brief Assignment from `std::basic_string_view`.
     */
    constexpr auto
    operator=(const std::basic_string_view<T>& str_view) noexcept -> RecordStringView&
    {
        if (this != &str_view) {
            std::basic_string_view<T>::operator=(str_view);
            m_codepoints.store(std::string_view::npos, std::memory_order_relaxed);
        }
        return *this;
    }

    /**
     * @brief Update pointer to the string data.
     *
     * Keeps number of codepoints untouched.
     *
     * @param data New data pointer.
     */
    auto update_data_ptr(const T* data) -> void
    {
        // Update only string view, codepoints remain the same.
        *static_cast<std::basic_string_view<T>*>(this) = {data, this->size()};
    }

    /**
     * @brief Calculate number of unicode code points.
     *
     * @return Number of code points.
     */
    auto codepoints() -> std::size_t
    {
        auto codepoints = m_codepoints.load(std::memory_order_consume);
        if (codepoints == std::string_view::npos) {
            codepoints = 0;
            if constexpr (sizeof(T) != 1) {
                codepoints = this->size();
            } else {
                const auto size = this->size();
                const auto data = this->data();
                for (std::size_t idx = 0; idx < size; codepoints++) {
                    idx += Util::Unicode::code_point_length(std::next(data, idx));
                }
            }
            m_codepoints.store(codepoints, std::memory_order_release);
        }
        return codepoints;
    }

private:
    std::atomic<std::size_t> m_codepoints = std::string_view::npos;
};

/**
 * @brief Deduction guide for a constructor call with a pointer and size.
 *
 * @tparam Char Char type for the string view.
 */
template<typename Char>
RecordStringView(const Char*, std::size_t) -> RecordStringView<Char>;

/**
 * @brief Log record.
 *
 * @tparam Char Char type.
 * @tparam String String type.
 */
template<typename Char, typename String>
struct Record {
    /**
     * @brief Source code location.
     */
    struct Location {
        RecordStringView<char> filename = {}; ///< File name.
        RecordStringView<char> function = {}; ///< Function name.
        std::size_t line = {}; ///< Line number.
    };

    Level level = {}; ///< Log level.
    Location location = {}; ///< Source code location.
    RecordStringView<Char> category = {}; ///< Log category.
    std::variant<std::reference_wrapper<String>, RecordStringView<Char>> message
        = RecordStringView<Char>{}; ///< Log message.
};

} // namespace PlainCloud::Log