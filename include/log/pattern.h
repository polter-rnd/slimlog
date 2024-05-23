#pragma once

#include "level.h"
#include "location.h"
#include "util.h"

#include <array>
#include <cstdlib>
#include <cuchar>
#include <cwchar>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace PlainCloud::Log {

#ifdef __cpp_lib_char8_t

namespace Detail {
namespace Char8Fallbacks {
template<typename T = char8_t>
auto mbrtoc8(T* /*unused*/, const char* /*unused*/, size_t /*unused*/, mbstate_t* /*unused*/)
    -> size_t
{
    static_assert(
        Util::AlwaysFalse<T>{},
        "C++ compiler does not support std::mbrtoc8(char8_t*, const char*, size_t, mbstate_t*)");
    return -1;
};

template<typename T = char8_t>
auto c8rtomb(char* /*unused*/, T /*unused*/, mbstate_t* /*unused*/) -> size_t
{
    static_assert(
        Util::AlwaysFalse<T>{},
        "C++ compiler does not support std::c8rtomb(char*, char8_t, mbstate_t*)");
    return -1;
}
} // namespace Char8Fallbacks

namespace Char8 {
using namespace Char8Fallbacks;
using namespace std;
} // namespace Char8

} // namespace Detail

#endif

template<typename Char>
class Pattern {
public:
    using CharType = Char;
    using StringType = typename std::basic_string_view<Char>;

    enum class Flag {
        Level = 'l',
        Topic = 't',
        Message = 'm',
        File = 'F',
        Line = 'L',
        Function = 'f'
    };

    struct Levels {
        StringType trace{DefaultTrace.data(), DefaultTrace.size()};
        StringType debug{DefaultDebug.data(), DefaultDebug.size()};
        StringType info{DefaultInfo.data(), DefaultInfo.size()};
        StringType warning{DefaultWarning.data(), DefaultWarning.size()};
        StringType error{DefaultError.data(), DefaultError.size()};
        StringType fatal{DefaultFatal.data(), DefaultFatal.size()};

    private:
        static constexpr std::array<Char, 5> DefaultTrace{'T', 'R', 'A', 'C', 'E'};
        static constexpr std::array<Char, 5> DefaultDebug{'D', 'E', 'B', 'U', 'G'};
        static constexpr std::array<Char, 4> DefaultInfo{'I', 'N', 'F', 'O'};
        static constexpr std::array<Char, 4> DefaultWarning{'W', 'A', 'R', 'N'};
        static constexpr std::array<Char, 5> DefaultError{'E', 'R', 'R', 'O', 'R'};
        static constexpr std::array<Char, 5> DefaultFatal{'F', 'A', 'T', 'A', 'L'};
    };

    template<typename... Args>
    explicit Pattern(const StringType& pattern = {}, Args&&... args)
        : m_pattern(pattern)
    {
        set_levels({std::forward<Args>(args)...});
    }

    [[nodiscard]] auto empty() const -> bool
    {
        return m_pattern.empty();
    }

    template<typename String>
    auto format(auto& result, const Level level, const String& category, const Location& caller)
        const -> void
    {
        if (empty()) {
            return;
        }

        StringType pattern{m_pattern};

        const auto result_pos = result.size();

        auto append = [&pattern, &result](auto pos, auto data) {
            using DataType = typename std::remove_cv_t<std::remove_pointer_t<decltype(data)>>;
            using IsDigit = std::is_arithmetic<decltype(data)>;
            using IsPointer = std::is_pointer<decltype(data)>;
            using ToWideChar = std::
                conjunction<std::is_same<DataType, char>, std::negation<std::is_same<Char, char>>>;
            using ToMultiByte = std::
                conjunction<std::is_same<Char, char>, std::negation<std::is_same<DataType, char>>>;

            result += pattern.substr(0, pos);

            if constexpr (IsPointer::value && ToWideChar::value) {
                to_widechar(result, data);
            } else if constexpr (IsPointer::value && ToMultiByte::value) {
                to_multibyte(result, data);
#ifdef __cpp_lib_char8_t
            } else if constexpr (std::is_same_v<Char, char8_t>) {
#endif
            } else if constexpr (std::is_same_v<Char, char16_t>) {
                result.format(u"{}", data);
            } else if constexpr (std::is_same_v<Char, char32_t>) {
                result.format(U"{}", data);
            } else if constexpr (std::is_same_v<Char, wchar_t>) {
                result.format(L"{}", data);
            } else {
                result.format("{}", data);
            }

            pattern = pattern.substr(pos + 2);
        };
        auto skip = [&pattern, &result](auto pos, auto flag) {
            result += pattern.substr(0, pos + 1);
            pattern = pattern.substr(pos + (flag == '%' ? 2 : 1));
        };

        for (;;) {
            const auto pos = pattern.find('%');
            const auto len = pattern.size();
            if (pos == pattern.npos || pos == len - 1) {
                result += pattern;
                break;
            }

            const auto chr = pattern.at(pos + 1);
            switch (static_cast<Flag>(chr)) {
            case Flag::Level:
                switch (level) {
                case Level::Fatal:
                    append(pos, m_levels.fatal);
                    break;
                case Level::Error:
                    append(pos, m_levels.error);
                    break;
                case Level::Warning:
                    append(pos, m_levels.warning);
                    break;
                case Level::Info:
                    append(pos, m_levels.info);
                    break;
                case Level::Debug:
                    append(pos, m_levels.debug);
                    break;
                case Level::Trace:
                    append(pos, m_levels.trace);
                    break;
                }
                break;
            case Flag::Topic:
                append(pos, category);
                break;
            case Flag::Message:
                append(pos, std::basic_string_view<CharType>(result.c_str(), result_pos));
                result.erase(0, result_pos);
                break;
            case Flag::File:
                append(pos, caller.file_name());
                break;
            case Flag::Line:
                append(pos, caller.line());
                break;
            case Flag::Function:
                append(pos, caller.function_name());
                break;
            default:
                skip(pos, chr);
                break;
            }
        }
    }

    auto set_pattern(StringType pattern)
    {
        m_pattern = pattern;
    }

    auto set_levels(const std::initializer_list<std::pair<Level, StringType>>& levels)
    {
        for (const auto& level : levels) {
            switch (level.first) {
            case Level::Fatal:
                m_levels.fatal = level.second;
                break;
            case Level::Error:
                m_levels.error = level.second;
                break;
            case Level::Warning:
                m_levels.warning = level.second;
                break;
            case Level::Info:
                m_levels.info = level.second;
                break;
            case Level::Debug:
                m_levels.debug = level.second;
                break;
            case Level::Trace:
                m_levels.trace = level.second;
                break;
            }
        }
    }

protected:
    template<typename T>
    static void to_widechar(auto& result, const T* data)
    {
        // Convert multi-byte to wide char
        Char wchr;
        auto len = std::char_traits<T>::length(data);
        auto state = std::mbstate_t();
        size_t (*towc_func)(Char*, const T*, size_t, mbstate_t*) = nullptr;
        if constexpr (std::is_same_v<Char, wchar_t>) {
            towc_func = std::mbrtowc;
#ifdef __cpp_lib_char8_t
        } else if constexpr (std::is_same_v<Char, char8_t>) {
            towc_func = Detail::Char8::mbrtoc8;
#endif
        } else if constexpr (std::is_same_v<Char, char16_t>) {
            towc_func = std::mbrtoc16;
        } else if constexpr (std::is_same_v<Char, char32_t>) {
            towc_func = std::mbrtoc32;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (int ret{}; (ret = towc_func(&wchr, data, len, &state)) > 0; data += ret, len -= ret) {
            result += wchr;
        }
    }

    template<typename T>
    static void to_multibyte(auto& result, const T* data)
    {
        // Convert wide char to multi-byte
        std::basic_string<Char> mbstr(MB_CUR_MAX, '\0');
        auto state = std::mbstate_t();
        auto len = std::char_traits<T>::length(data);
        size_t (*tomb_func)(Char*, T, mbstate_t*) = nullptr;
        if constexpr (std::is_same_v<T, wchar_t>) {
            tomb_func = std::wcrtomb;
#ifdef __cpp_lib_char8_t
        } else if constexpr (std::is_same_v<T, char8_t>) {
            tomb_func = Detail::Char8::c8rtomb;
#endif
        } else if constexpr (std::is_same_v<T, char16_t>) {
            tomb_func = std::c16rtomb;
        } else if constexpr (std::is_same_v<T, char32_t>) {
            tomb_func = std::c32rtomb;
        }
        for (int ret{}; (ret = tomb_func(mbstr.data(), *data, &state)) > 0 && len != 0;
             data++, len--) {
            result.write(mbstr.c_str(), ret);
        }
    }

private:
    std::basic_string<Char> m_pattern;
    Levels m_levels;
};

} // namespace PlainCloud::Log
