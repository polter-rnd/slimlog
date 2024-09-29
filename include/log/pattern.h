/**
 * @file pattern.h
 * @brief Contains definition of Pattern class.
 */

#pragma once

#include "level.h"
#include "location.h"
#include "logger.h"
#include "record.h"
#include "util/types.h"
#include "util/unicode.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cuchar>
#include <cwchar>
#include <initializer_list>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace PlainCloud::Log {

/** @cond */
namespace Detail {
#ifdef __cpp_lib_char8_t

namespace Char8Fallback {
template<typename T = char8_t>
auto mbrtoc8(T* /*unused*/, const char* /*unused*/, size_t /*unused*/, mbstate_t* /*unused*/)
    -> size_t
{
    static_assert(
        Util::Types::AlwaysFalse<T>{},
        "C++ compiler does not support std::mbrtoc8(char8_t*, const char*, size_t, mbstate_t*)");
    return -1;
};
} // namespace Char8Fallback

namespace Char8 {
using namespace Char8Fallback;
using namespace std;
} // namespace Char8

#endif
} // namespace Detail
/** @endcond */

/**
 * @brief Convert from `std::string` to logger string type.
 *
 * Specialize this function for logger string type to be able
 * to use compile-time formatting for non-standard string types.
 *
 * @tparam String Type of logger string.
 * @param str Original string.
 * @return Converted string.
 */
template<typename Char, typename String>
[[maybe_unused]] constexpr auto to_string_view(const String& str) -> std::basic_string_view<Char>;

/**
 * @brief Log message pattern.
 *
 * @tparam Char Char type for pattern string.
 */
template<typename Char>
class Pattern {
public:
    /** @brief String type for pattern (`std::basic_string_view<Char>`). */
    using StringViewType = typename std::basic_string_view<Char>;

    /** @brief Log level names */
    struct Levels {
        auto get(Level level) -> RecordStringView<Char>&
        {
            switch (level) {
            case Level::Fatal:
                return m_fatal;
            case Level::Error:
                return m_error;
            case Level::Warning:
                return m_warning;
            case Level::Info:
                return m_info;
            case Level::Debug:
                return m_debug;
            case Level::Trace:
                [[fallthrough]];
            default:
                return m_trace;
            }
        }

        auto set(Level level, StringViewType name) -> void
        {
            switch (level) {
            case Level::Fatal:
                m_fatal = std::move(name);
                break;
            case Level::Error:
                m_error = std::move(name);
                break;
            case Level::Warning:
                m_warning = std::move(name);
                break;
            case Level::Info:
                m_info = std::move(name);
                break;
            case Level::Debug:
                m_debug = std::move(name);
                break;
            case Level::Trace:
                m_trace = std::move(name);
                break;
            }
        }

    private:
        static constexpr std::array<Char, 6> DefaultTrace{'T', 'R', 'A', 'C', 'E', '\0'};
        static constexpr std::array<Char, 6> DefaultDebug{'D', 'E', 'B', 'U', 'G', '\0'};
        static constexpr std::array<Char, 5> DefaultInfo{'I', 'N', 'F', 'O', '\0'};
        static constexpr std::array<Char, 5> DefaultWarning{'W', 'A', 'R', 'N', '\0'};
        static constexpr std::array<Char, 6> DefaultError{'E', 'R', 'R', 'O', 'R', '\0'};
        static constexpr std::array<Char, 6> DefaultFatal{'F', 'A', 'T', 'A', 'L', '\0'};

        RecordStringView<Char> m_trace{DefaultTrace.data()}; ///< Trace level
        RecordStringView<Char> m_debug{DefaultDebug.data()}; ///< Debug level
        RecordStringView<Char> m_info{DefaultInfo.data()}; ///< Info level
        RecordStringView<Char> m_warning{DefaultWarning.data()}; ///< Warning level
        RecordStringView<Char> m_error{DefaultError.data()}; ///< Error level
        RecordStringView<Char> m_fatal{DefaultFatal.data()}; ///< Fatal level
    };

    struct Placeholder {
        /** @brief Log pattern placeholder types */
        enum class Type { None, Category, Level, File, Line, Message };

        enum class Align { None, Left, Right, Center };

        struct StringSpecs {
            size_t width = 0;
            Align align = Align::None;
            StringViewType fill = StringSpecs::DefaultFill.data();

        private:
            static constexpr std::array<Char, 2> DefaultFill{' ', '\0'};
        };

        static auto is_string(Type type) -> bool
        {
            switch (type) {
            case Placeholder::Type::Category:
                [[fallthrough]];
            case Placeholder::Type::Level:
                [[fallthrough]];
            case Placeholder::Type::File:
                [[fallthrough]];
            case Placeholder::Type::Message:
                return true;
            default:
                break;
            }
            return false;
        }

        Type type = Type::None;
        std::variant<StringViewType, StringSpecs> value = StringViewType{};
    };

    struct Placeholders {
    private:
        static constexpr std::array<Char, 9> Category{'c', 'a', 't', 'e', 'g', 'o', 'r', 'y', '\0'};
        static constexpr std::array<Char, 6> Level{'l', 'e', 'v', 'e', 'l', '\0'};
        static constexpr std::array<Char, 5> File{'f', 'i', 'l', 'e', '\0'};
        static constexpr std::array<Char, 5> Line{'l', 'i', 'n', 'e', '\0'};
        static constexpr std::array<Char, 8> Message{'m', 'e', 's', 's', 'a', 'g', 'e', '\0'};

    public:
        static constexpr std::array<Placeholder, 5> List{
            {{Placeholder::Type::Category, Placeholders::Category.data()},
             {Placeholder::Type::Level, Placeholders::Level.data()},
             {Placeholder::Type::File, Placeholders::File.data()},
             {Placeholder::Type::Line, Placeholders::Line.data()},
             {Placeholder::Type::Message, Placeholders::Message.data()}}};
    };

    /**
     * @brief Construct a new Pattern object.
     *
     * Usage:
     *
     * ```cpp
     * Log::Pattern<char> pattern(
     *       "(%t) [%l] %F|%L: %m",
     *       std::make_pair(Log::Level::Trace, "Trace"),
     *       std::make_pair(Log::Level::Debug, "Debug"),
     *       std::make_pair(Log::Level::Info, "Info"),
     *       std::make_pair(Log::Level::Warning, "Warn"),
     *       std::make_pair(Log::Level::Error, "Error"),
     *       std::make_pair(Log::Level::Fatal, "Fatal"));
     * ```
     *
     * @tparam Args Argument types for log level pairs.
     * @param pattern Pattern string.
     * @param args Log level pairs.
     */
    template<typename... Args>
    explicit Pattern(StringViewType pattern = {}, Args&&... args)
    {
        set_levels({std::forward<Args>(args)...});
        compile(pattern);
    }

    /**
     * @brief Check if pattern is empty.
     *
     * @return \b true if pattern is an empty string.
     * @return \b false if pattern is not an empty string.
     */
    [[nodiscard]] auto empty() const -> bool
    {
        return m_pattern.empty();
    }

    /**
     * @brief Format message according to the pattern.
     *
     * @tparam String %Logger string type.
     * @param result Buffer storing the raw message to be overwritten with result.
     * @param level Log level.
     * @param category %Logger category name.
     * @param location Caller location (file, line, function).
     */
    template<typename StringType>
    auto format(auto& result, Record<Char, StringType>& record) -> void
    {
        if (empty()) {
            // Empty format, just append message
            std::visit(
                Util::Types::Overloaded{
                    [&result](std::reference_wrapper<StringType> arg) {
                        if constexpr (std::is_convertible_v<StringType, StringViewType>) {
                            result.append(arg.get());
                        } else {
                            result.append(to_string_view<Char>(arg.get()));
                        }
                    },
                    [&result](auto&& arg) { result.append(std::forward<decltype(arg)>(arg)); }},
                record.message);
        } else {
            using StringSpecs = typename Placeholder::StringSpecs;

            // Process format placeholders
            for (auto& item : m_placeholders) {
                switch (item.type) {
                case Placeholder::Type::None:
                    result.append(std::get<StringViewType>(item.value));
                    break;
                case Placeholder::Type::Category:
                    format_string(result, std::get<StringSpecs>(item.value), record.category);
                    break;
                case Placeholder::Type::Level:
                    format_string(
                        result, std::get<StringSpecs>(item.value), m_levels.get(record.level));
                    break;
                case Placeholder::Type::File:
                    format_string(
                        result, std::get<StringSpecs>(item.value), record.location.filename);
                    break;
                case Placeholder::Type::Line:
                    result.format_runtime(
                        std::get<StringViewType>(item.value), record.location.line);
                    break;
                case Placeholder::Type::Message:
                    std::visit(
                        Util::Types::Overloaded{
                            [this, &result, &specs = std::get<StringSpecs>(item.value)](
                                std::reference_wrapper<StringType> arg) {
                                if constexpr (std::is_convertible_v<StringType, StringViewType>) {
                                    RecordStringView message{StringViewType{arg.get()}};
                                    this->format_string(result, specs, message);
                                } else {
                                    RecordStringView message{to_string_view<Char>(arg.get())};
                                    this->format_string(result, specs, message);
                                }
                            },
                            [this, &result, &specs = std::get<StringSpecs>(item.value)](
                                auto&& arg) {
                                this->format_string(
                                    result, specs, std::forward<decltype(arg)>(arg));
                            },
                        },
                        record.message);
                    break;
                default:
                    break;
                }
            }
        }
    }

    /**
     * @brief Set the message pattern.
     *
     * Usage example:
     *
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_pattern("(%t) [%l] %F|%L: %m");
     * ```
     *
     * @param pattern Message pattern.
     */
    auto set_pattern(StringViewType pattern) -> void
    {
        compile(pattern);
    }

    /**
     * @brief Set the log level names.
     *
     * Set a name for each log level. Usage example:
     *
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_levels({{Log::Level::Info,
     * "Information"}});
     * ```
     *
     * @param levels Initializer list of log level pairs.
     */
    auto set_levels(std::initializer_list<std::pair<Level, StringViewType>> levels) -> void
    {
        for (const auto& level : levels) {
            m_levels.set(level.first, level.second);
        }
    }

protected:
    constexpr auto
    parse_nonnegative_int(const Char*& begin, const Char* end, int error_value) noexcept -> int
    {
        unsigned value = 0;
        unsigned prev = 0;
        auto ptr = begin;
        constexpr auto Base = 10ULL;
        while (ptr != end && '0' <= *ptr && *ptr <= '9') {
            prev = value;
            value = value * Base + static_cast<unsigned>(*ptr - '0');
            ++ptr;
        }
        auto num_digits = ptr - begin;
        begin = ptr;

        constexpr auto Digits10 = static_cast<int>(sizeof(int) * CHAR_BIT * 3 / Base);
        if (num_digits <= Digits10) {
            return static_cast<int>(value);
        }

        // Check for overflow.
        constexpr auto MaxInt = std::numeric_limits<int>::max();
        return num_digits == Digits10 + 1
                && prev * Base + static_cast<unsigned>(*std::prev(ptr) - '0') <= MaxInt
            ? static_cast<int>(value)
            : error_value;
    }

    constexpr auto
    parse_align(const Char* begin, const Char* end, Placeholder::StringSpecs& specs) -> const Char*
    {
        auto align = Placeholder::Align::None;
        auto* ptr = std::next(begin, Util::Unicode::code_point_length(begin));
        if (end - ptr <= 0) {
            ptr = begin;
        }

        for (;;) {
            switch (Util::Unicode::to_ascii(*ptr)) {
            case '<':
                align = Placeholder::Align::Left;
                break;
            case '>':
                align = Placeholder::Align::Right;
                break;
            case '^':
                align = Placeholder::Align::Center;
                break;
            }
            if (align != Placeholder::Align::None) {
                if (ptr != begin) {
                    // Actually this check is redundant, cause using '{' or '}'
                    // as a fill character will cause parsing failure earlier.
                    switch (Util::Unicode::to_ascii(*begin)) {
                    case '}':
                        return begin;
                    case '{':
                        throw FormatError("format: invalid fill character '{'\n");
                    }
                    specs.fill = std::basic_string_view<Char>(
                        begin,
                        static_cast<std::make_unsigned_t<decltype(ptr - begin)>>(ptr - begin));
                    begin = std::next(ptr);
                } else {
                    std::advance(begin, 1);
                }
                break;
            }
            if (ptr == begin) {
                break;
            }
            ptr = begin;
        }
        specs.align = align;

        return begin;
    }

    auto compile(StringViewType pattern)
    {
        m_placeholders.clear();
        m_pattern.clear();
        m_pattern.reserve(pattern.size());

        auto append_item = [this](Placeholder::Type type, size_t count, size_t shift = 0) {
            auto data = StringViewType{m_pattern}.substr(m_pattern.size() - count - shift, count);
            // In case of raw text, we can safely merge current chunk with the last one
            if (type == Placeholder::Type::None && !m_placeholders.empty()
                && m_placeholders.back().type == type) {
                auto& format = std::get<StringViewType>(m_placeholders.back().value);
                format = StringViewType{format.data(), format.size() + data.size()};
            } else {
                m_placeholders.emplace_back(type, data);
            }
        };

        bool inside_placeholder = false;
        for (;;) {
            static constexpr std::array<Char, 3> Delimeters{'{', '}', '\0'};
            const auto pos = pattern.find_first_of(Delimeters.data());
            const auto len = pattern.size();

            if (pos == pattern.npos) {
                // Append leftovers, if any
                if (inside_placeholder) {
                    throw FormatError("format error: unmatched '{' in format string");
                }
                if (!pattern.empty()) {
                    m_pattern.append(pattern);
                    append_item(Placeholder::Type::None, len);
                }
                break;
            }

            const auto chr = Util::Unicode::to_ascii(pattern[pos]);

            // Handle escaped braces
            if (!inside_placeholder && pos < len - 1
                && chr == static_cast<char>(pattern[pos + 1])) {
                m_pattern.append(pattern.substr(0, pos + 1));
                pattern = pattern.substr(pos + 2);
                append_item(Placeholder::Type::None, pos + 1);
                continue;
            }

            if (!inside_placeholder && chr == '{') {
                // Enter inside placeholder
                m_pattern.append(pattern.substr(0, pos + 1));
                pattern = pattern.substr(pos + 1);
                append_item(Placeholder::Type::None, pos, 1);

                inside_placeholder = true;
            } else if (inside_placeholder && chr == '}') {
                // Leave the placeholder
                Placeholder placeholder;
                for (const auto& item : Placeholders::List) {
                    if (pattern.starts_with(std::get<StringViewType>(item.value))) {
                        placeholder = item;
                        break;
                    }
                }

                auto delta = std::get<StringViewType>(placeholder.value).size();
                auto type = placeholder.type;

                m_pattern.append(pattern.substr(delta, pos + 1 - delta));
                pattern = pattern.substr(pos + 1);

                // Save empty string view instead of {} to mark unformatted placeholder
                append_item(type, pos - delta + 2);

                inside_placeholder = false;
            } else {
                // Unescaped brace found
                throw FormatError(
                    std::string("format error: unmatched '") + chr + "' in format string");
                break;
            }
        }

        // Calculate formatted width for string fields
        for (auto& placeholder : m_placeholders) {
            if (!Placeholder::is_string(placeholder.type)) {
                continue;
            }

            typename Placeholder::StringSpecs specs = {};
            auto& value = std::get<StringViewType>(placeholder.value);
            if (value.size() > 2) {
                auto* fmt = parse_align(value.begin() + 2, value.end(), specs);
                if (auto chr = Util::Unicode::to_ascii(*fmt); chr != '}') {
                    const int width = parse_nonnegative_int(fmt, value.end() - 1, -1);
                    if (width == -1) {
                        throw FormatError("format field width is too big");
                    }
                    chr = Util::Unicode::to_ascii(*fmt);
                    switch (chr) {
                    case '}':
                        break;
                    case 's':
                        if (Util::Unicode::to_ascii(*std::next(fmt)) != '}') {
                            throw FormatError("missing '}' in format string");
                        }
                        break;
                    default:
                        throw FormatError(
                            std::string("wrong format type '") + chr + "' for the string field");
                    }
                    specs.width = width;
                }
            }

            placeholder.value = specs;
        }
    }

    /**
     * @brief Convert from multi-byte to single-byte string
     *
     * @tparam T Char type
     * @param result Destination stream buffer
     * @param data Source string
     */
    template<typename T>
    static void from_multibyte(auto& result, std::basic_string_view<T> data)
    {
        Char wchr;
        auto state = std::mbstate_t{};
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

        for (int ret{};
             (ret = static_cast<int>(towc_func(&wchr, data.data(), data.size(), &state))) > 0;
             data = data.substr(ret)) {
            result.push_back(wchr);
        }
    }

    template<typename T>
    constexpr auto
    write_padded(auto& dst, RecordStringView<T>& src, const Placeholder::StringSpecs& specs)
    {
        const auto spec_width
            = static_cast<std::make_unsigned_t<decltype(specs.width)>>(specs.width);
        const auto width = src.codepoints();
        const size_t padding = spec_width > width ? spec_width - width : 0;

        // Shifts are encoded as string literals because static constexpr is not
        // supported in constexpr functions.
        const char* shifts = "\x1f\x1f\x00\x01";
        const size_t left_padding
            = padding >> static_cast<unsigned>(*std::next(shifts, static_cast<int>(specs.align)));
        const size_t right_padding = padding - left_padding;

        // Reserve exact amount for data + padding
        dst.reserve(dst.size() + src.size() + padding * specs.fill.size());

        // Lambda for filling with single character or multibyte pattern
        auto fill_pattern = [&dst, &fill = specs.fill](size_t fill_len) {
            const auto* src = fill.data();
            auto block_size = fill.size();
            auto* dest = std::prev(dst.end(), fill_len * block_size);

            if (block_size > 1) {
                // Copy first block
                std::copy_n(src, block_size, dest);

                // Copy other blocks recursively via O(n*log(N)) calls
                const auto* start = dest;
                const auto* end = std::next(start, fill_len * block_size);
                auto* current = std::next(dest, block_size);
                while (std::next(current, block_size) < end) {
                    std::copy_n(start, block_size, current);
                    std::advance(current, block_size);
                    block_size *= 2;
                }

                // Copy the rest
                std::copy_n(start, end - current, current);
            } else {
                std::fill_n(dest, fill_len, *src);
            }
        };

        // Fill left padding
        if (left_padding != 0) {
            dst.resize(dst.size() + left_padding * specs.fill.size());
            fill_pattern(left_padding);
        }

        // Fill data
        dst.append(src);

        // Fill right padding
        if (right_padding != 0) {
            dst.resize(dst.size() + right_padding * specs.fill.size());
            fill_pattern(right_padding);
        }
    }

    template<typename T>
    inline auto format_string(
        auto& result, const Placeholder::StringSpecs& specs, RecordStringView<T>& data) -> void
    {
        if (specs.width > 0) {
            this->write_padded(result, data, specs);
        } else {
            if constexpr (std::is_same_v<T, char> && !std::is_same_v<Char, char>) {
                this->from_multibyte(result, data);
            } else {
                // Special case: since formatted message is stored
                // at the beginning of the buffer, we have to reserve
                // capacity first to prevent changing buffer address
                // while re-allocating later.
                result.reserve(result.size() + data.size());
                result.append(data);
            }
        }
    }

private:
    std::basic_string<Char> m_pattern;
    std::vector<Placeholder> m_placeholders;
    Levels m_levels;
};

} // namespace PlainCloud::Log
