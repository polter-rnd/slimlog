/**
 * @file pattern.h
 * @brief Contains the definition of the Pattern class.
 */

#pragma once

#include "format.h"
#include "level.h"
#include "record.h"
#include "util/types.h"
#include "util/unicode.h"

#include <array>
#include <climits>
#include <cstdint>
#include <cuchar>
#include <cwchar>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace PlainCloud::Log {

/** @cond */
namespace Detail {
#ifdef __cpp_lib_char8_t

namespace Char8Fallback {
template<typename T = char8_t>
auto mbrtoc8(T* /*unused*/, const char* /*unused*/, std::size_t /*unused*/, mbstate_t* /*unused*/)
    -> std::size_t
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
 * @brief Converts a string to a `std::basic_string_view`.
 *
 * This function takes a String object and returns a `std::basic_string_view`
 * of the same character type.
 *
 * @tparam Char Type of the characters in the string.
 * @tparam String String type.
 * @param str The input string object to be converted.
 * @return A `std::basic_string_view` of the same character type as the input string.
 */
template<typename Char, typename String>
[[maybe_unused]] constexpr auto to_string_view(const String& str) -> std::basic_string_view<Char>;

/**
 * @brief Represents a log message pattern specifying the message format.
 *
 * This class defines the pattern used for formatting log messages.
 *
 * @tparam Char The character type for the pattern string.
 */
template<typename Char>
class Pattern {
public:
    /** @brief String type for pattern (`std::basic_string_view<Char>`). */
    using StringViewType = typename std::basic_string_view<Char>;

    /**
     * @brief Struct for managing log level names.
     */
    struct Levels {
        /**
         * @brief Retrieves the name of the specified log level.
         *
         * @param level The log level.
         * @return A reference to the name of the specified log level.
         */
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

        /**
         * @brief Sets the name for the specified log level.
         *
         * @param level The log level.
         * @param name The new name for the log level.
         */
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

    /**
     * @brief %Placeholder for message pattern.
     *
     * Specifies available fields for message pattern.
     */
    struct Placeholder {
        /** @brief Log pattern field types. */
        enum class Type : std::uint8_t { None, Category, Level, File, Line, Message };

        /** @brief Field alignment options. */
        enum class Align : std::uint8_t { None, Left, Right, Center };

        /** @brief Parameters for string fields. */
        struct StringSpecs {
            std::size_t width = 0; ///< Field width.
            Align align = Align::None; ///< Field alignment.
            StringViewType fill = StringSpecs::DefaultFill.data(); ///< Fill character.

        private:
            static constexpr std::array<Char, 2> DefaultFill{' ', '\0'};
        };

        /**
         * @brief Checks if the placeholder is for a string.
         *
         * @param type Placeholder type.
         * @return \b true if the placeholder is a string.
         * @return \b false if the placeholder is not a string.
         */
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

        Type type = Type::None; ///< Placeholder type.
        std::variant<StringViewType, StringSpecs> value = StringViewType{}; ///< Placeholder value.
    };

    /**
     * @brief %Placeholder field names.
     */
    struct Placeholders {
    private:
        static constexpr std::array<Char, 9> Category{'c', 'a', 't', 'e', 'g', 'o', 'r', 'y', '\0'};
        static constexpr std::array<Char, 6> Level{'l', 'e', 'v', 'e', 'l', '\0'};
        static constexpr std::array<Char, 5> File{'f', 'i', 'l', 'e', '\0'};
        static constexpr std::array<Char, 5> Line{'l', 'i', 'n', 'e', '\0'};
        static constexpr std::array<Char, 8> Message{'m', 'e', 's', 's', 'a', 'g', 'e', '\0'};

    public:
        /** @brief List of placeholder names. */
        static constexpr std::array<Placeholder, 5> List{
            {{Placeholder::Type::Category, Placeholders::Category.data()},
             {Placeholder::Type::Level, Placeholders::Level.data()},
             {Placeholder::Type::File, Placeholders::File.data()},
             {Placeholder::Type::Line, Placeholders::Line.data()},
             {Placeholder::Type::Message, Placeholders::Message.data()}}};
    };

    /**
     * @brief Constructs a new Pattern object.
     *
     * This constructor initializes a Pattern object with a specified pattern string
     * and optional log level pairs.
     *
     * Usage example:
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
     * @brief Checks if the pattern is empty.
     *
     * @return \b true if the pattern is an empty string.
     * @return \b false if the pattern is not an empty string.
     */
    [[nodiscard]] auto empty() const -> bool
    {
        return m_pattern.empty();
    }

    /**
     * @brief Formats a message according to the pattern.
     *
     * This function formats a log message based on the specified pattern.
     *
     * @tparam StringType %Logger string type.
     * @param result Buffer storing the raw message to be overwritten with the result.
     * @param record Log record.
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
     * @brief Sets the message pattern.
     *
     * This function sets the pattern used for formatting log messages.
     *
     * Usage example:
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
     * @brief Sets the log level names.
     *
     * This function sets a name for each log level.
     *
     * Usage example:
     * ```cpp
     * Log::Logger log("test", Log::Level::Info);
     * log.add_sink<Log::OStreamSink>(std::cerr)->set_levels({{Log::Level::Info, "Information"}});
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
    /**
     * @brief Compiles the pattern string into a fast-lookup representation.
     *
     * This function processes the provided pattern string and converts it into
     * an internal representation that allows for efficient formatting of log messages.
     *
     * @param pattern The pattern string to be compiled.
     */
    void compile(StringViewType pattern)
    {
        m_placeholders.clear();
        m_pattern.clear();
        m_pattern.reserve(pattern.size());

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
                    append_placeholder(Placeholder::Type::None, len);
                }
                break;
            }

            const auto chr = Util::Unicode::to_ascii(pattern[pos]);

            // Handle escaped braces
            if (!inside_placeholder && pos < len - 1
                && chr == static_cast<char>(pattern[pos + 1])) {
                m_pattern.append(pattern.substr(0, pos + 1));
                pattern = pattern.substr(pos + 2);
                append_placeholder(Placeholder::Type::None, pos + 1);
                continue;
            }

            if (!inside_placeholder && chr == '{') {
                // Enter inside placeholder
                m_pattern.append(pattern.substr(0, pos + 1));
                pattern = pattern.substr(pos + 1);
                append_placeholder(Placeholder::Type::None, pos, 1);

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
                append_placeholder(type, pos - delta + 2);

                inside_placeholder = false;
            } else {
                // Unescaped brace found
                throw FormatError(
                    std::string("format error: unmatched '") + chr + "' in format string");
                break;
            }
        }
    }

    /**
     * @brief Converts a multi-byte string to a single-byte string.
     *
     * This function converts a multi-byte string to a single-byte string and appends the result to
     * the provided destination stream buffer.
     *
     * @tparam T Character type of the source string.
     * @param result Destination stream buffer where the converted string will be appended.
     * @param data Source multi-byte string to be converted.
     */
    template<typename T>
    static void from_multibyte(auto& result, std::basic_string_view<T> data)
    {
        Char wchr;
        auto state = std::mbstate_t{};
        std::size_t (*towc_func)(Char*, const T*, std::size_t, mbstate_t*) = nullptr;
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

    /**
     * @brief Formats a string according to the specified specifications.
     *
     * This function formats the source string based on the provided specifications,
     * including alignment and fill character, and appends the result to the given buffer.
     *
     * @tparam T Character type for the string.
     * @param result Buffer where the formatted string will be appended.
     * @param specs Specifications for the string formatting (alignment and fill character).
     * @param data Source string to be formatted.
     */
    template<typename T>
    auto format_string(
        auto& result, const Placeholder::StringSpecs& specs, RecordStringView<T>& data) -> void
    {
        if (specs.width > 0) {
            this->write_padded(result, data, specs);
        } else {
            if constexpr (std::is_same_v<T, char> && !std::is_same_v<Char, char>) {
                this->from_multibyte(result, data); // NOLINT(cppcoreguidelines-slicing)
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
    /**
     * @brief Converts a string to a non-negative integer.
     *
     * This function parses a string and converts it to a non-negative integer.
     *
     * @param begin Reference to a pointer to the beginning of the string.
     *              The pointer will be advanced by the number of characters processed.
     * @param end Pointer to the past-the-end character of the string.
     * @param error_value The default value to return in case of a conversion error.
     * @return The parsed integer, or error_value if the conversion fails.
     */
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
        const auto num_digits = ptr - begin;
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

    /**
     * @brief Parses alignment (^, <, >) and fill character from the formatting field.
     *
     * This function parses the alignment and fill character from the given string
     * and updates the provided specs structure with the parsed values.
     *
     * @param begin Pointer to the beginning of the string.
     * @param end Pointer to the past-the-end of the string.
     * @param specs Reference to the output specs structure to be updated.
     * @return Pointer to the past-the-end of the processed characters.
     */
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

    /**
     * @brief Append a pattern placeholder to the list of placeholders.
     *
     * This function parses a placeholder from the end of the string and appends it to the list of
     * placeholders.
     *
     * @param type Placeholder type.
     * @param count Placeholder length.
     * @param shift Margin from the end of the pattern string.
     */
    void append_placeholder(Placeholder::Type type, std::size_t count, std::size_t shift = 0)
    {
        auto data = StringViewType{m_pattern}.substr(m_pattern.size() - count - shift, count);
        if (type == Placeholder::Type::None && !m_placeholders.empty()
            && m_placeholders.back().type == type) {
            // In case of raw text, we can safely merge current chunk with the last one
            auto& format = std::get<StringViewType>(m_placeholders.back().value);
            format = StringViewType{format.data(), format.size() + data.size()};
        } else if (Placeholder::is_string(type)) {
            // Calculate formatted width for string fields
            m_placeholders.emplace_back(type, get_string_specs(data));
        } else {
            m_placeholders.emplace_back(type, data);
        }
    };

    /**
     * @brief Parses the string specifications from the formatting field.
     *
     * This function extracts the string specifications, such as width, alignment, and fill
     * character, from the given placeholder field value.
     *
     * @param value The string value of the placeholder field.
     * @return A Placeholder::StringSpecs object containing the parsed specifications.
     */
    auto get_string_specs(StringViewType value) -> Placeholder::StringSpecs
    {
        typename Placeholder::StringSpecs specs = {};
        if (value.size() > 2) {
            const auto* begin = value.data();
            const auto* end = std::next(begin, value.size());
            const auto* fmt = parse_align(std::next(begin, 2), end, specs);
            if (auto chr = Util::Unicode::to_ascii(*fmt); chr != '}') {
                const int width = parse_nonnegative_int(fmt, std::prev(end), -1);
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
        return specs;
    }

    /**
     * @brief Writes the source string to the destination buffer with specific alignment.
     *
     * This function writes the source string to the destination buffer, applying the specified
     * alignment and fill character.
     *
     * @tparam T Character type for the string view.
     * @param dst Destination buffer where the string will be written.
     * @param src Source string view to be written.
     * @param specs String specifications, including alignment and fill character.
     */
    template<typename T>
    constexpr void
    write_padded(auto& dst, RecordStringView<T>& src, const Placeholder::StringSpecs& specs)
    {
        const auto spec_width
            = static_cast<std::make_unsigned_t<decltype(specs.width)>>(specs.width);
        const auto width = src.codepoints();
        const auto padding = spec_width > width ? spec_width - width : 0;

        // Shifts are encoded as string literals because static constexpr is not
        // supported in constexpr functions.
        const char* shifts = "\x1f\x1f\x00\x01";
        const auto left_padding
            = padding >> static_cast<unsigned>(*std::next(shifts, static_cast<int>(specs.align)));
        const auto right_padding = padding - left_padding;

        // Reserve exact amount for data + padding
        dst.reserve(dst.size() + src.size() + padding * specs.fill.size());

        // Lambda for filling with single character or multibyte pattern
        auto fill_pattern = [&dst, &fill = specs.fill](std::size_t fill_len) {
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

    std::basic_string<Char> m_pattern;
    std::vector<Placeholder> m_placeholders;
    Levels m_levels;
};

} // namespace PlainCloud::Log
