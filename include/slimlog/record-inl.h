/**
 * @file record-inl.h
 * @brief Contains the definition of the RecordStringView class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/record.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/record.h" // IWYU pragma: associated
#include "slimlog/util/unicode.h"

#include <atomic>
#include <utility>

namespace SlimLog {

template<typename T>
RecordStringView<T>::RecordStringView(const RecordStringView& str_view) noexcept
    : std::basic_string_view<T>(str_view)
    , m_codepoints(str_view.m_codepoints)
{
}

template<typename T>
RecordStringView<T>::RecordStringView(RecordStringView&& str_view) noexcept
    : std::basic_string_view<T>(std::move(static_cast<std::basic_string_view<T>&&>(str_view)))
    , m_codepoints(str_view.m_codepoints)
{
}

template<typename T>
auto RecordStringView<T>::operator=(const RecordStringView& str_view) noexcept -> RecordStringView&
{
    if (this == &str_view) {
        return *this;
    }

    std::basic_string_view<T>::operator=(str_view);
    m_codepoints = str_view.m_codepoints;
    return *this;
}

template<typename T>
auto RecordStringView<T>::operator=(RecordStringView&& str_view) noexcept -> RecordStringView&
{
    if (this == &str_view) {
        return *this;
    }

    std::basic_string_view<T>::operator=(
        std::move(static_cast<std::basic_string_view<T>&&>(str_view)));
    m_codepoints = str_view.m_codepoints;
    return *this;
}

template<typename T>
auto RecordStringView<T>::operator=(std::basic_string_view<T> str_view) noexcept
    -> RecordStringView&
{
    std::basic_string_view<T>::operator=(std::move(str_view));
    m_codepoints = std::string_view::npos;
    return *this;
}

template<typename T>
auto RecordStringView<T>::codepoints() const noexcept -> std::size_t
{
    if (auto expected = std::string_view::npos; m_codepoints == expected) {
        // Thread-safe lazy initialization using std::atomic_ref
        // in case of possible concurrent access from multiple sinks
        const auto calculated = Util::Unicode::count_codepoints(this->data(), this->size());
#ifdef __cpp_lib_atomic_ref
        const std::atomic_ref atomic_codepoints{m_codepoints};
#else
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto& atomic_codepoints = *reinterpret_cast<std::atomic<std::size_t>*>(&m_codepoints);
#endif
        atomic_codepoints.compare_exchange_strong(expected, calculated, std::memory_order_relaxed);
    }
    return m_codepoints;
}

} // namespace SlimLog
