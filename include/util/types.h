/**
 * @file util.h
 * @brief Various utility classes and functions.
 */

#pragma once

#include <type_traits>

namespace PlainCloud::Util::Types {

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template<typename>
struct AlwaysFalse : std::false_type { };

template<typename T>
struct UnderlyingChar {
    static_assert(AlwaysFalse<T>{}, "Unable to deduce the underlying char type");
};

template<typename T>
    requires std::is_integral_v<typename std::remove_cvref_t<T>::value_type>
struct UnderlyingChar<T> {
    using Type = typename std::remove_cvref_t<T>::value_type;
};

template<typename T>
    requires std::is_array_v<std::remove_cvref_t<T>>
struct UnderlyingChar<T> {
    using Type = typename std::remove_cvref_t<typename std::remove_all_extents_t<T>>;
};

template<typename T>
    requires std::is_integral_v<typename std::remove_pointer_t<T>>
struct UnderlyingChar<T> {
    using Type = typename std::remove_cvref_t<typename std::remove_pointer_t<T>>;
};

template<typename T>
using UnderlyingCharType = typename UnderlyingChar<T>::Type;

} // namespace PlainCloud::Util::Types
