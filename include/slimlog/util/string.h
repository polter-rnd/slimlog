/**
 * @file string.h
 * @brief Provides utility classes for string manipulation.
 */

#pragma once

#include "slimlog/util/unicode.h"

#include <atomic>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace SlimLog {

/**
 * @brief Non-owning string view type with cached codepoints.
 *
 * This class extends `std::basic_string_view<T>` and includes a `codepoints()` method
 * to calculate and cache the number of Unicode code points in a string. It is implicitly
 * convertible from `std::basic_string_view<T>` and `std::basic_string<T>`.
 *
 * @tparam T Character type.
 */
template<typename T>
class CachedStringView : public std::basic_string_view<T> {
public:
    using std::basic_string_view<T>::basic_string_view;

    ~CachedStringView() = default;

    /**
     * @brief Default constructor.
     */
    constexpr CachedStringView() noexcept
        : std::basic_string_view<T>()
    {
    }

    /**
     * @brief Copy constructor.
     *
     * @param str_view The CachedStringView to copy from.
     */
    constexpr CachedStringView(const CachedStringView& str_view) noexcept
        : std::basic_string_view<T>(str_view)
        , m_codepoints_local(str_view.m_codepoints_local)
        , m_codepoints_external(str_view.m_codepoints_external)
    {
    }

    /**
     * @brief Move constructor.
     *
     * @param str_view The CachedStringView to move from.
     */
    constexpr CachedStringView(CachedStringView&& str_view) noexcept
        : std::basic_string_view<T>(std::move(static_cast<std::basic_string_view<T>&&>(str_view)))
        , m_codepoints_local(str_view.m_codepoints_local)
        , m_codepoints_external(str_view.m_codepoints_external)
    {
    }

    /**
     * @brief Constructor from `std::basic_string_view`.
     *
     * @param str_view The std::basic_string_view to construct from.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    constexpr CachedStringView(std::basic_string_view<T> str_view) noexcept
        : std::basic_string_view<T>(std::move(str_view))
    {
    }

    /**
     * @brief Constructor from `std::basic_string`.
     *
     * @param str The std::basic_string to construct from.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    constexpr CachedStringView(const std::basic_string<T>& str) noexcept
        : std::basic_string_view<T>(str)
    {
    }

    /**
     * @brief Assignment operator.
     *
     * @param str_view The CachedStringView to assign from.
     * @return Reference to this CachedStringView.
     */
    auto operator=(const CachedStringView& str_view) noexcept -> CachedStringView&
    {
        if (this == &str_view) {
            return *this;
        }

        std::basic_string_view<T>::operator=(str_view);
        m_codepoints_local = str_view.m_codepoints_local;
        m_codepoints_external = str_view.m_codepoints_external;
        return *this;
    }

    /**
     * @brief Move assignment operator.
     *
     * @param str_view The CachedStringView to move from.
     * @return Reference to this CachedStringView.
     */
    auto operator=(CachedStringView&& str_view) noexcept -> CachedStringView&
    {
        if (this == &str_view) {
            return *this;
        }

        std::basic_string_view<T>::operator=(
            std::move(static_cast<std::basic_string_view<T>&&>(str_view)));
        m_codepoints_local = str_view.m_codepoints_local;
        m_codepoints_external = str_view.m_codepoints_external;
        return *this;
    }

    /**
     * @brief Assignment from `std::basic_string_view`.
     *
     * @param str_view The std::basic_string_view to assign from.
     * @return Reference to this CachedStringView.
     */
    auto operator=(const std::basic_string_view<T>& str_view) noexcept -> CachedStringView&
    {
        std::basic_string_view<T>::operator=(str_view);
        m_codepoints_local = std::string_view::npos;
        m_codepoints_external = nullptr;
        return *this;
    }

    /**
     * @brief Calculate the number of Unicode code points.
     *
     * @return Number of code points.
     */
    auto codepoints() const noexcept -> std::size_t
    {
        std::size_t* codepoints_ptr
            = m_codepoints_external != nullptr ? m_codepoints_external : &m_codepoints_local;
        if (auto expected = std::string_view::npos; *codepoints_ptr == expected) {
            // Thread-safe lazy initialization using std::atomic_ref
            // in case of possible concurrent access from multiple sinks
            const auto calculated = Util::Unicode::count_codepoints(this->data(), this->size());
#ifdef __cpp_lib_atomic_ref
            const std::atomic_ref atomic_codepoints{*codepoints_ptr};
#else
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto& atomic_codepoints = *reinterpret_cast<std::atomic<std::size_t>*>(codepoints_ptr);
#endif
            atomic_codepoints.compare_exchange_strong(
                expected, calculated, std::memory_order_relaxed);
        }
        return *codepoints_ptr;
    }

private:
    template<typename U>
    friend class CachedString;

    mutable std::size_t m_codepoints_local = std::string_view::npos;
    mutable std::size_t* m_codepoints_external = nullptr;
};

/**
 * @brief Owning string type with cached codepoints.
 *
 * This class extends `std::basic_string<T>` and is implicitly convertible to
 * `CachedStringView<T>`. It maintains a cached codepoints count that is preserved
 * across copy/move operations and transferred to CachedStringView on conversion.
 *
 * @tparam T Character type.
 */
template<typename T>
class CachedString : public std::basic_string<T> {
public:
    using std::basic_string<T>::basic_string;

    /**
     * @brief Default destructor.
     */
    ~CachedString() = default;

    /**
     * @brief Copy constructor - preserves cached codepoints.
     *
     * @param other The CachedString to copy from.
     */
    CachedString(const CachedString& other)
        : std::basic_string<T>(other)
        , m_codepoints(other.m_codepoints)
    {
    }

    /**
     * @brief Move constructor - preserves cached codepoints.
     *
     * @param other The CachedString to move from.
     */
    CachedString(CachedString&& other) noexcept
        : std::basic_string<T>(std::move(other))
        , m_codepoints(other.m_codepoints)
    {
    }

    /**
     * @brief Constructor from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to construct from.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    CachedString(const std::basic_string<T>& str)
        : std::basic_string<T>(str)
    {
    }

    /**
     * @brief Move constructor from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to move from.
     */
    // NOLINTNEXTLINE(*-explicit-conversions)
    CachedString(std::basic_string<T>&& str) noexcept
        : std::basic_string<T>(std::move(str))
    {
    }

    /**
     * @brief Copy assignment - preserves cached codepoints.
     *
     * @param other The CachedString to assign from.
     * @return Reference to this CachedString.
     */
    auto operator=(const CachedString& other) -> CachedString&
    {
        if (this != &other) {
            std::basic_string<T>::operator=(other);
            m_codepoints = other.m_codepoints;
        }
        return *this;
    }

    /**
     * @brief Move assignment - preserves cached codepoints.
     *
     * @param other The CachedString to move from.
     * @return Reference to this CachedString.
     */
    auto operator=(CachedString&& other) noexcept -> CachedString&
    {
        if (this != &other) {
            std::basic_string<T>::operator=(std::move(other));
            m_codepoints = other.m_codepoints;
        }
        return *this;
    }

    /**
     * @brief Assignment from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to assign from.
     * @return Reference to this CachedString.
     */
    auto operator=(const std::basic_string<T>& str) -> CachedString&
    {
        std::basic_string<T>::operator=(str);
        m_codepoints = std::string_view::npos;
        return *this;
    }

    /**
     * @brief Move assignment from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to move from.
     * @return Reference to this CachedString.
     */
    auto operator=(std::basic_string<T>&& str) noexcept -> CachedString&
    {
        std::basic_string<T>::operator=(std::move(str));
        m_codepoints = std::string_view::npos;
        return *this;
    }

    /**
     * @brief Assignment from CachedStringView - preserves cached codepoints.
     *
     * @param view The CachedStringView to assign from.
     * @return Reference to this CachedString.
     */
    auto operator=(const CachedStringView<T>& view) -> CachedString&
    {
        std::basic_string<T>::operator=(static_cast<std::basic_string_view<T>>(view));
        m_codepoints = *view.m_codepoints;
        return *this;
    }

    /**
     * @brief Assignment from std::basic_string_view - invalidates codepoints cache.
     *
     * @param view The std::basic_string_view to assign from.
     * @return Reference to this CachedString.
     */
    auto operator=(std::basic_string_view<T> view) -> CachedString&
    {
        std::basic_string<T>::operator=(view);
        m_codepoints = std::string_view::npos;
        return *this;
    }

    /**
     * @brief Implicit conversion to CachedStringView.
     *
     * The returned view shares the codepoints cache with this CachedString,
     * so calculating codepoints in either object updates both.
     *
     * @return A CachedStringView that references this CachedString.
     */
    operator CachedStringView<T>() const noexcept // NOLINT(hicpp-explicit-conversions)
    {
        CachedStringView<T> view(static_cast<const std::basic_string<T>&>(*this));
        view.m_codepoints_external = &m_codepoints;
        return view;
    }

    /**
     * @brief Calculate and cache the number of Unicode code points.
     *
     * Uses the same thread-safe lazy initialization as CachedStringView.
     *
     * @return Number of code points.
     */
    auto codepoints() const noexcept -> std::size_t
    {
        if (m_codepoints == std::string_view::npos) {
            CachedStringView<T> view(*this);
            m_codepoints = view.codepoints();
        }
        return m_codepoints;
    }

private:
    mutable std::size_t m_codepoints = std::string_view::npos;
};

/**
 * @brief Deduction guide for a constructor call with a pointer and size.
 *
 * @tparam Char Character type for the string view.
 */
template<typename Char>
CachedStringView(const Char*, std::size_t) -> CachedStringView<Char>;

} // namespace SlimLog
