#pragma once

#include <array>
#include <string>

template<typename Char>
inline auto test_string() -> std::basic_string<Char>
{
    static constexpr std::array<Char, 13> HelloWorld
        = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
    return {HelloWorld.data(), HelloWorld.size()};
}

template<typename String>
inline auto test_string_out() -> String
{
    return String{test_string<String>()} + static_cast<typename String::value_type>('\n');
}
