/**
 * @file string.h
 * @brief Provides utility classes for string manipulation.
 */

#pragma once

#include "slimlog/util/unicode.h"

#include <atomic>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace SlimLog {

/**
 * @brief Non-owning string view type with cached codepoints.
 *
 * This class extends `std::basic_string_view<T>` and includes a `codepoints()` method
 * to calculate and cache the number of Unicode code points in a string. It is explicitly
 * convertible from `std::basic_string_view<T>` and `std::basic_string<T>`.
 *
 * @tparam T Character type.
 * @tparam Traits Character traits.
 */
template<typename T, typename Traits = std::char_traits<T>>
class CachedStringView : public std::basic_string_view<T, Traits> {
private:
    using BaseType = std::basic_string_view<T, Traits>;

public:
    using BaseType::basic_string_view;

    /** @brief Default destructor. */
    ~CachedStringView() = default;
    /** @brief Move constructor.*/
    constexpr CachedStringView(CachedStringView&&) noexcept = default;
    /** Move assignment operator. */
    auto operator=(CachedStringView&&) noexcept -> CachedStringView& = default;

    /**
     * @brief Copy constructor.
     *
     * @param str_view The CachedStringView to copy from.
     */
    constexpr CachedStringView(const CachedStringView& str_view) noexcept
        : BaseType(str_view)
        , m_codepoints_local(str_view.m_codepoints_local)
        , m_codepoints_external(str_view.m_codepoints_external)
    {
    }

    /**
     * @brief Constructor from `std::basic_string_view`.
     *
     * @param str_view The std::basic_string_view to construct from.
     */
    explicit constexpr CachedStringView(BaseType str_view) noexcept
        : BaseType(str_view)
    {
    }

    /**
     * @brief Copy assignment operator.
     *
     * @param str_view The CachedStringView to assign from.
     * @return Reference to this CachedStringView.
     */
    auto operator=(const CachedStringView& str_view) noexcept -> CachedStringView&
    {
        if (this == &str_view) {
            return *this;
        }

        BaseType::operator=(str_view);
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
    auto operator=(const BaseType& str_view) noexcept -> CachedStringView&
    {
        BaseType::operator=(str_view);
        m_codepoints_local = BaseType::npos;
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
        if (auto expected = BaseType::npos; *codepoints_ptr == expected) {
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
    template<typename U, typename UTraits, typename UAllocator>
    friend class CachedString;

    mutable std::size_t m_codepoints_local = BaseType::npos;
    mutable std::size_t* m_codepoints_external = nullptr;
};

/**
 * @brief Deduction guide for a constructor call with a pointer.
 *
 * @tparam Char Character type for the string view.
 */
template<typename Char>
CachedStringView(const Char*) -> CachedStringView<Char>;

/**
 * @brief Deduction guide for a constructor call with a pointer and size.
 *
 * @tparam Char Character type for the string view.
 */
template<typename Char>
CachedStringView(const Char*, std::size_t) -> CachedStringView<Char>;

/**
 * @brief Deduction guide for iterator range constructor.
 *
 * @tparam InputIt Iterator type.
 */
template<typename InputIt>
CachedStringView(InputIt, InputIt)
    -> CachedStringView<typename std::iterator_traits<InputIt>::value_type>;

/**
 * @brief Owning string type with cached codepoints.
 *
 * This class wraps `std::basic_string<T>` and is explicitly convertible to
 * `CachedStringView<T>`. It maintains a cached codepoints count that is preserved
 * across copy/move operations and transferred to CachedStringView on conversion.
 *
 * The string is immutable after construction to ensure cache validity.
 *
 * @tparam T Character type.
 * @tparam Traits Character traits.
 * @tparam Allocator Allocator type.
 */
template<typename T, typename Traits = std::char_traits<T>, typename Allocator = std::allocator<T>>
class CachedString : private std::basic_string<T, Traits, Allocator> {
private:
    using BaseType = std::basic_string<T, Traits, Allocator>;

public:
    // Type aliases
    using typename BaseType::allocator_type;
    using typename BaseType::const_iterator;
    using typename BaseType::const_pointer;
    using typename BaseType::const_reference;
    using typename BaseType::const_reverse_iterator;
    using typename BaseType::difference_type;
    using typename BaseType::size_type;
    using typename BaseType::traits_type;
    using typename BaseType::value_type;

    // Iterators
    using BaseType::cbegin;
    using BaseType::cend;
    using BaseType::crbegin;
    using BaseType::crend;

    // Capacity
    using BaseType::capacity;
    using BaseType::empty;
    using BaseType::length;
    using BaseType::max_size;
    using BaseType::size;

    // Search
    using BaseType::find;
    using BaseType::find_first_not_of;
    using BaseType::find_first_of;
    using BaseType::find_last_not_of;
    using BaseType::find_last_of;
    using BaseType::npos;
    using BaseType::rfind;

    // Operations
    using BaseType::compare;
    using BaseType::ends_with;
    using BaseType::starts_with;
    using BaseType::substr;

    // Allocator
    using BaseType::get_allocator;

    // Constructor
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    using BaseType::basic_string;

    /** @brief Default destructor. */
    ~CachedString() = default;

    /**
     * @brief Copy constructor - preserves cached codepoints.
     *
     * @param other The CachedString to copy from.
     */
    constexpr CachedString(const CachedString& other)
        : BaseType(other)
        , m_codepoints(other.m_codepoints)
    {
    }

    /**
     * @brief Move constructor - preserves cached codepoints.
     *
     * @param other The CachedString to move from.
     */
    constexpr CachedString(CachedString&& other) noexcept
        : BaseType(static_cast<BaseType&&>(other))
        , m_codepoints(std::exchange(other.m_codepoints, BaseType::npos))
    {
    }

    /**
     * @brief Copy constructor with allocator - preserves cached codepoints.
     *
     * @param other The CachedString to copy from.
     * @param alloc The allocator to use.
     */
    constexpr CachedString(const CachedString& other, const Allocator& alloc)
        : BaseType(other, alloc)
        , m_codepoints(other.m_codepoints)
    {
    }

    /**
     * @brief Move constructor with allocator - preserves cached codepoints.
     *
     * @param other The CachedString to move from.
     * @param alloc The allocator to use.
     */
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    constexpr CachedString(CachedString&& other, const Allocator& alloc)
        : BaseType(static_cast<BaseType&&>(other), alloc)
        , m_codepoints(std::exchange(other.m_codepoints, BaseType::npos))
    {
    }

    /**
     * @brief Constructor from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to construct from.
     */
    explicit CachedString(const BaseType& str)
        : BaseType(str)
    {
    }

    /**
     * @brief Move constructor from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to move from.
     */
    explicit CachedString(BaseType&& str) noexcept
        : BaseType(std::move(str))
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
            BaseType::operator=(other);
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
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    auto operator=(CachedString&& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
        || std::allocator_traits<Allocator>::is_always_equal::value) -> CachedString&
    {
        if (this != &other) {
            BaseType::operator=(static_cast<BaseType&&>(other));
            m_codepoints = std::exchange(other.m_codepoints, BaseType::npos);
        }
        return *this;
    }

    /**
     * @brief Assignment from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to assign from.
     * @return Reference to this CachedString.
     */
    auto operator=(const BaseType& str) -> CachedString&
    {
        BaseType::operator=(str);
        m_codepoints = BaseType::npos;
        return *this;
    }

    /**
     * @brief Move assignment from std::basic_string - invalidates codepoints cache.
     *
     * @param str The std::basic_string to move from.
     * @return Reference to this CachedString.
     */
    auto operator=(BaseType&& str) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
        || std::allocator_traits<Allocator>::is_always_equal::value) -> CachedString&
    {
        BaseType::operator=(std::move(str));
        m_codepoints = BaseType::npos;
        return *this;
    }

    /**
     * @brief Explicit conversion to CachedStringView.
     *
     * The returned view shares the codepoints cache with this CachedString,
     * so calculating codepoints in either object updates both.
     *
     * @return A CachedStringView that references this CachedString.
     */
    explicit operator CachedStringView<T, Traits>() const noexcept
    {
        CachedStringView<T, Traits> view(static_cast<const BaseType&>(*this));
        view.m_codepoints_external = &m_codepoints;
        return view;
    }

    /**
     * @brief Explicit conversion to std::basic_string_view.
     *
     * @return A string_view that references this CachedString.
     */
    explicit operator std::basic_string_view<T, Traits>() const noexcept
    {
        return std::basic_string_view<T, Traits>(static_cast<const BaseType&>(*this));
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
        if (m_codepoints == BaseType::npos) {
            const CachedStringView<T, Traits> view(*this);
            m_codepoints = view.codepoints();
        }
        return m_codepoints;
    }

private:
    mutable std::size_t m_codepoints = BaseType::npos;
};

/**
 * @brief Deduction guide for iterator range constructor.
 *
 * @tparam InputIt Iterator type.
 * @tparam Allocator Allocator type.
 */
template<
    typename InputIt,
    typename Allocator = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
CachedString(InputIt, InputIt, Allocator = Allocator()) // For clang-format < 19
    ->CachedString<
        typename std::iterator_traits<InputIt>::value_type,
        std::char_traits<typename std::iterator_traits<InputIt>::value_type>,
        Allocator>;

/**
 * @brief Deduction guide for C-string with count constructor.
 *
 * @tparam Char Character type.
 * @tparam Allocator Allocator type.
 */
template<typename Char, typename Allocator = std::allocator<Char>>
CachedString(const Char*, std::size_t, Allocator = Allocator())
    -> CachedString<Char, std::char_traits<Char>, Allocator>;

/**
 * @brief Deduction guide for null-terminated C-string constructor.
 *
 * @tparam Char Character type.
 * @tparam Allocator Allocator type.
 */
template<typename Char, typename Allocator = std::allocator<Char>>
CachedString(const Char*, Allocator = Allocator())
    -> CachedString<Char, std::char_traits<Char>, Allocator>;

/**
 * @brief Deduction guide for string_view-like constructor.
 *
 * @tparam StringViewLike String view-like type.
 * @tparam Allocator Allocator type.
 */
template<
    typename StringViewLike,
    typename Allocator = std::allocator<typename StringViewLike::value_type>>
CachedString(const StringViewLike&, Allocator = Allocator()) // For clang-format < 19
    ->CachedString<
        typename StringViewLike::value_type,
        typename StringViewLike::traits_type,
        Allocator>;

/**
 * @brief Deduction guide for string_view-like substring constructor.
 *
 * @tparam StringViewLike String view-like type.
 * @tparam Allocator Allocator type.
 */
template<
    typename StringViewLike,
    typename Allocator = std::allocator<typename StringViewLike::value_type>>
CachedString(const StringViewLike&, std::size_t, std::size_t, Allocator = Allocator())
    -> CachedString<
        typename StringViewLike::value_type,
        typename StringViewLike::traits_type,
        Allocator>;

} // namespace SlimLog
