/**
 * @file types.h
 * @brief Provides various utility classes and functions.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace slimlog::util::types {

/**
 * @brief Helper type to create an overloaded function object.
 *
 * Combines multiple lambdas or function objects into a single callable entity.
 * Useful with `std::visit` when working with `std::variant`.
 *
 * Example usage:
 * ```cpp
 * std::visit(Overloaded{
 *     [](auto arg) { std::cout << arg << ' '; },
 *     [](double arg) { std::cout << std::fixed << arg << ' '; },
 *     [](const std::string& arg) { std::cout << std::quoted(arg) << ' '; }
 * }, v);
 * ```
 *
 * @tparam Ts Variant types.
 */
template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

/** @brief Deduction guide for Overloaded. */
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

/**
 * @brief Defines a type that is always false for unconditional static assertions.
 *
 * Example usage:
 * ```cpp
 * static_assert(util::types::AlwaysFalse<T>{}, "Assertion failed");
 * ```
 */
template<typename>
struct AlwaysFalse : std::false_type {};

/// @cond
template<typename T>
struct UnderlyingChar {
    static_assert(AlwaysFalse<T>{}, "Unable to deduce the underlying char type");
};
/// @endcond

/**
 * @brief Detects the underlying char type of a string.
 *
 * @tparam T String type.
 */
template<typename T>
    requires std::is_integral_v<typename std::remove_cvref_t<T>::value_type>
struct UnderlyingChar<T> {
    /** @brief Defines the underlying char type, specialization for STL strings. */
    using Type = typename std::remove_cvref_t<T>::value_type;
};

template<typename T>
    requires std::is_array_v<std::remove_cvref_t<T>>
struct UnderlyingChar<T> {
    /** @brief Defines the underlying char type, specialization for char arrays. */
    using Type = typename std::remove_cvref_t<typename std::remove_all_extents_t<T>>;
};

template<typename T>
    requires std::is_integral_v<typename std::remove_pointer_t<T>>
struct UnderlyingChar<T> {
    /** @brief Defines the underlying char type, specialization for char pointers. */
    using Type = typename std::remove_cvref_t<typename std::remove_pointer_t<T>>;
};

/**
 * @brief Provides an alias for UnderlyingChar<T>::Type.
 *
 * @tparam T String type.
 */
template<typename T>
using UnderlyingCharType = typename UnderlyingChar<T>::Type;

/**
 * @brief Casts a nonnegative integer to unsigned.
 *
 * @tparam Int Integer type.
 *
 * @param value Non-negative integer value.
 */
template<typename Int>
constexpr auto to_unsigned(Int value) -> std::make_unsigned_t<Int>
{
    assert(std::is_unsigned_v<Int> || value >= 0);
    return static_cast<std::make_unsigned_t<Int>>(value);
}

/**
 * @brief Helper to iterate over all types in a variant.
 */
template<typename Variant, typename Func>
static constexpr void variant_for_each_type(const Func& func)
{
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (func.template operator()<std::variant_alternative_t<I, Variant>>(), ...);
    }(std::make_index_sequence<std::variant_size_v<Variant>>{});
}

} // namespace slimlog::util::types
