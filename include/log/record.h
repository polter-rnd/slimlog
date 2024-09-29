/**
 * @file record.h
 * @brief Contains definition of Record class.
 */

#pragma once

#include "level.h"
#include "util/unicode.h"

#include <atomic>
#include <iostream>
#include <iterator>
#include <string_view>
#include <variant>

namespace PlainCloud::Log {

template<typename T>
class RecordStringView : public std::basic_string_view<T> {
public:
    using std::basic_string_view<T>::basic_string_view;

    constexpr RecordStringView(const RecordStringView& str_view) noexcept
        : std::basic_string_view<T>(str_view)
        , m_codepoints(str_view.m_codepoints.load(std::memory_order_relaxed))
    {
    }

    // NOLINTNEXTLINE(*-explicit-conversions)
    constexpr RecordStringView(const std::basic_string_view<T>& str_view) noexcept
        : std::basic_string_view<T>(str_view)
    {
    }

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

    constexpr auto
    operator=(const std::basic_string_view<T>& str_view) noexcept -> RecordStringView&
    {
        if (this != &str_view) {
            std::basic_string_view<T>::operator=(str_view);
            m_codepoints.store(std::string_view::npos, std::memory_order_relaxed);
        }
        return *this;
    }

    auto update_data_ptr(const T* data) -> void
    {
        // Update only string view, codepoints remain the same.
        *static_cast<std::basic_string_view<T>*>(this) = {data, this->size()};
    }

    auto codepoints() -> size_t
    {
        size_t codepoints = m_codepoints.load(std::memory_order_consume);
        if (codepoints == std::string_view::npos) {
            codepoints = 0;
            if constexpr (sizeof(T) != 1) {
                codepoints = this->size();
            } else {
                const auto size = this->size();
                const auto data = this->data();
                for (size_t idx = 0; idx < size; codepoints++) {
                    idx += Util::Unicode::code_point_length(std::next(data, idx));
                }
            }
            m_codepoints.store(codepoints, std::memory_order_release);
        }
        return codepoints;
    }

private:
    std::atomic<size_t> m_codepoints = std::string_view::npos;
};

template<typename Char>
RecordStringView(const Char*, size_t) -> RecordStringView<Char>;

template<typename Char, typename StringType>
struct Record {
    struct Location {
        // NOLINTNEXTLINE(*-explicit-conversions)
        Location(Log::Location location)
            : filename(location.file_name())
            , function(location.function_name())
            , line(location.line())
        {
        }

        RecordStringView<char> filename;
        RecordStringView<char> function;
        size_t line;
    };

    Level level = {};
    Location location = {};
    RecordStringView<Char> category = {};
    std::variant<std::reference_wrapper<StringType>, RecordStringView<Char>> message
        = RecordStringView<Char>{};
};

} // namespace PlainCloud::Log