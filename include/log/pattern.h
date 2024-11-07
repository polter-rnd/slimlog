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
#include <concepts>
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
struct ConvertString {
    auto operator()(const String&) const -> std::basic_string_view<Char> = delete;
};

namespace Detail {
template<typename Char, typename StringType>
concept HasConvertString = requires(StringType value) {
    { ConvertString<Char, StringType>{}(value) } -> std::same_as<std::basic_string_view<Char>>;
};
} // namespace Detail

/**
 * @brief Represents a log message pattern specifying the message format.
 *
 * This class defines how log messages are formatted using placeholders.
 *
 * @tparam Char The character type for the pattern string.
 */
template<typename Char>
class Pattern {
public:
    /** @brief String view type for pattern strings. */
    using StringViewType = std::basic_string_view<Char>;

    /**
     * @brief Structure for managing log level names.
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
     * @brief Placeholder for message pattern components.
     *
     * Specifies available fields and their formatting options.
     */
    struct Placeholder {
        /** @brief Log pattern field types. */
        enum class Type : std::uint8_t {
            None,
            Category,
            Level,
            File,
            Line,
            Message,
            Function,
            Time,
            Msec,
            Usec,
            Nsec,
            Thread
        };

        /** @brief Parameters for string fields. */
        struct StringSpecs {
            /** @brief Field alignment options. */
            enum class Align : std::uint8_t { None, Left, Right, Center };

            std::size_t width = 0; ///< Field width.
            Align align = Align::None; ///< Field alignment.
            StringViewType fill = StringSpecs::DefaultFill.data(); ///< Fill character.

        private:
            static constexpr std::array<Char, 2> DefaultFill{' ', '\0'};
        };

        Type type = Type::None; ///< Placeholder type.
        std::variant<
            StringViewType,
            StringSpecs,
            CachedFormatter<std::size_t, Char>,
            CachedFormatter<RecordTime::TimePoint, Char>>
            value = StringViewType{}; ///< Placeholder value.
    };

    /**
     * @brief Placeholder field names.
     */
    struct Placeholders {
    private:
        static constexpr std::array<Char, 9> Category{'c', 'a', 't', 'e', 'g', 'o', 'r', 'y', '\0'};
        static constexpr std::array<Char, 6> Level{'l', 'e', 'v', 'e', 'l', '\0'};
        static constexpr std::array<Char, 5> File{'f', 'i', 'l', 'e', '\0'};
        static constexpr std::array<Char, 5> Line{'l', 'i', 'n', 'e', '\0'};
        static constexpr std::array<Char, 9> Function{'f', 'u', 'n', 'c', 't', 'i', 'o', 'n', '\0'};
        static constexpr std::array<Char, 5> Time{'t', 'i', 'm', 'e', '\0'};
        static constexpr std::array<Char, 5> Msec{'m', 's', 'e', 'c', '\0'};
        static constexpr std::array<Char, 5> Usec{'u', 's', 'e', 'c', '\0'};
        static constexpr std::array<Char, 5> Nsec{'n', 's', 'e', 'c', '\0'};
        static constexpr std::array<Char, 7> Thread{'t', 'h', 'r', 'e', 'a', 'd', '\0'};
        static constexpr std::array<Char, 8> Message{'m', 'e', 's', 's', 'a', 'g', 'e', '\0'};

    public:
        /** @brief List of placeholder names. */
        static constexpr std::array<std::pair<typename Placeholder::Type, StringViewType>, 11> List{
            {{Placeholder::Type::Category, Placeholders::Category.data()},
             {Placeholder::Type::Level, Placeholders::Level.data()},
             {Placeholder::Type::File, Placeholders::File.data()},
             {Placeholder::Type::Line, Placeholders::Line.data()},
             {Placeholder::Type::Function, Placeholders::Function.data()},
             {Placeholder::Type::Time, Placeholders::Time.data()},
             {Placeholder::Type::Msec, Placeholders::Msec.data()},
             {Placeholder::Type::Usec, Placeholders::Usec.data()},
             {Placeholder::Type::Nsec, Placeholders::Nsec.data()},
             {Placeholder::Type::Thread, Placeholders::Thread.data()},
             {Placeholder::Type::Message, Placeholders::Message.data()}}};
    };

    /**
     * @brief Constructs a new Pattern object.
     *
     * Initializes with a pattern string and optional log level pairs.
     *
     * Usage example:
     * ```cpp
     * Log::Pattern<char> pattern(
     *       "({category}) [{level}] {file}|{line}: {message}",
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
     * @param args Log level and name pairs.
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
     * @param out Buffer storing the raw message to be overwritten with the result.
     * @param record Log record.
     */
    template<typename StringType>
    auto format(auto& out, Record<Char, StringType>& record) -> void
    {
        constexpr std::size_t MsecInNsec = 1000000;
        constexpr std::size_t UsecInNsec = 1000;

        // Process format placeholders
        for (auto& item : m_placeholders) {
            switch (item.type) {
            case Placeholder::Type::None:
                out.append(std::get<StringViewType>(item.value));
                break;
            case Placeholder::Type::Category:
                format_string(out, item.value, record.category);
                break;
            case Placeholder::Type::Level:
                format_string(out, item.value, m_levels.get(record.level));
                break;
            case Placeholder::Type::File:
                format_string(out, item.value, record.location.filename);
                break;
            case Placeholder::Type::Function:
                format_string(out, item.value, record.location.function);
                break;
            case Placeholder::Type::Line:
                format_generic(out, item.value, record.location.line);
                break;
            case Placeholder::Type::Time:
                format_generic(out, item.value, record.time.local);
                break;
            case Placeholder::Type::Msec:
                format_generic(out, item.value, record.time.nsec / MsecInNsec);
                break;
            case Placeholder::Type::Usec:
                format_generic(out, item.value, record.time.nsec / UsecInNsec);
                break;
            case Placeholder::Type::Nsec:
                format_generic(out, item.value, record.time.nsec);
                break;
            case Placeholder::Type::Thread:
                format_generic(out, item.value, record.thread_id);
                break;
            case Placeholder::Type::Message:
                std::visit(
                    Util::Types::Overloaded{
                        [&out, &value = item.value](std::reference_wrapper<StringType> arg) {
                            if constexpr (Detail::HasConvertString<Char, StringType>) {
                                format_string(
                                    out, value, ConvertString<Char, StringType>{}(arg.get()));
                            } else {
                                (void)out;
                                (void)value;
                                (void)arg;
                                throw FormatError(
                                    "No corresponding Log::ConvertString<> specialization found");
                            }
                        },
                        [&out, &value = item.value](auto&& arg) {
                            format_string(out, value, std::forward<decltype(arg)>(arg));
                        },
                    },
                    record.message);
                break;
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
                    throw FormatError("format error: unmatched '{' in pattern string");
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
                && chr == Util::Unicode::to_ascii(pattern[pos + 1])) {
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
                auto placeholder = std::find_if(
                    Placeholders::List.begin(),
                    Placeholders::List.end(),
                    [pattern](const auto& item) { return pattern.starts_with(item.second); });
                if (placeholder == Placeholders::List.end()) {
                    throw FormatError("format error: unknown pattern placeholder found");
                }

                const auto delta = placeholder->second.size();
                m_pattern.append(pattern.substr(delta, pos + 1 - delta));
                pattern = pattern.substr(pos + 1);
                append_placeholder(placeholder->first, pos - delta);

                inside_placeholder = false;
            } else {
                // Unescaped brace found
                throw FormatError(
                    std::string("format error: unmatched '") + chr + "' in pattern string");
                break;
            }
        }

        // If no placeholders found, just add message as a default
        if (m_placeholders.empty()) {
            m_placeholders.emplace_back(
                Placeholder::Type::Message, typename Placeholder::StringSpecs{});
        }
    }

    /**
     * @brief Converts a multi-byte string to a single-byte string.
     *
     * This function converts a multi-byte string to a single-byte string and appends the result to
     * the provided destination stream buffer.
     *
     * @tparam T Character type of the source string.
     * @param out Destination stream buffer where the converted string will be appended.
     * @param data Source multi-byte string to be converted.
     */
    template<typename T>
    static void from_multibyte(auto& out, std::basic_string_view<T> data)
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
            out.push_back(wchr);
        }
    }

    /**
     * @brief Formats a string according to the specifications.
     *
     * This function formats the source string based on the provided specifications,
     * including alignment and fill character, and appends the result to the given buffer.
     *
     * @tparam StringView Source string type (can be std::string_view or RecordStringView).
     * @param out Buffer where the formatted string will be appended.
     * @param item Variant holding either StringSpecs or RecordStringView.
     * @param data Source string to be formatted.
     */
    template<typename StringView>
    static void format_string(auto& out, const auto& item, StringView&& data)
    {
        if (auto& specs = std::get<typename Placeholder::StringSpecs>(item); specs.width > 0)
            [[unlikely]] {
            write_padded(out, std::forward<StringView>(data), specs);
        } else {
            using DataChar = typename std::remove_cvref_t<StringView>::value_type;
            if constexpr (std::is_same_v<DataChar, char> && !std::is_same_v<Char, char>) {
                // NOLINTNEXTLINE (cppcoreguidelines-slicing)
                from_multibyte(out, std::forward<StringView>(data));
            } else {
                out.append(std::forward<StringView>(data));
            }
        }
    }

    /**
     * @brief Formats data according to the cached format context.
     *
     * This function formats the source data based on the format context from CachedFormatter,
     * including alignment and fill character, and appends the out to the given buffer.
     *
     * @tparam T Data type.
     * @param out Buffer where the formatted data will be appended.
     * @param item Variant holding CachedFormatter<T, Char>, where T is the data type.
     * @param data Source data to be formatted.
     */
    template<typename T>
    static void format_generic(auto& out, const auto& item, T data)
    {
        std::get<CachedFormatter<T, Char>>(item).format(out, data);
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
    static constexpr auto
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
    static constexpr auto
    parse_align(const Char* begin, const Char* end, Placeholder::StringSpecs& specs) -> const Char*
    {
        auto align = Placeholder::StringSpecs::Align::None;
        auto* ptr = std::next(begin, Util::Unicode::code_point_length(begin));
        if (end - ptr <= 0) {
            ptr = begin;
        }

        for (;;) {
            switch (Util::Unicode::to_ascii(*ptr)) {
            case '<':
                align = Placeholder::StringSpecs::Align::Left;
                break;
            case '>':
                align = Placeholder::StringSpecs::Align::Right;
                break;
            case '^':
                align = Placeholder::StringSpecs::Align::Center;
                break;
            }
            if (align != Placeholder::StringSpecs::Align::None) {
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
                        begin, Util::Types::to_unsigned(ptr - begin));
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
     * This function parses a placeholder from the end of the string and appends it to the list
     * of placeholders.
     *
     * @param type Placeholder type.
     * @param count Placeholder length.
     * @param shift Margin from the end of the pattern string.
     */
    void append_placeholder(Placeholder::Type type, std::size_t count, std::size_t shift = 0)
    {
        auto data = StringViewType{m_pattern}.substr(m_pattern.size() - count - shift, count);
        switch (type) {
        case Placeholder::Type::None:
            if (!m_placeholders.empty() && m_placeholders.back().type == type) {
                // In case of raw text, we can safely merge current chunk with the last one
                auto& format = std::get<StringViewType>(m_placeholders.back().value);
                format = StringViewType{format.data(), format.size() + data.size()};
            } else {
                // Otherwise just add new string placeholder
                m_placeholders.emplace_back(type, data);
            }
            break;
        case Placeholder::Type::Category:
        case Placeholder::Type::Level:
        case Placeholder::Type::File:
        case Placeholder::Type::Function:
        case Placeholder::Type::Message:
            m_placeholders.emplace_back(type, get_string_specs(data));
            break;
        case Placeholder::Type::Line:
        case Placeholder::Type::Thread:
        case Placeholder::Type::Msec:
        case Placeholder::Type::Usec:
        case Placeholder::Type::Nsec:
            m_placeholders.emplace_back(type, CachedFormatter<std::size_t, Char>(data));
            break;
        case Placeholder::Type::Time:
            m_placeholders.emplace_back(type, CachedFormatter<RecordTime::TimePoint, Char>(data));
            break;
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
    static auto get_string_specs(StringViewType value) -> Placeholder::StringSpecs
    {
        typename Placeholder::StringSpecs specs = {};
        if (!value.empty()) {
            const auto* begin = value.data();
            const auto* end = std::next(begin, value.size());
            const auto* fmt = parse_align(begin, end, specs);
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
    template<typename StringView>
    constexpr static void
    write_padded(auto& dst, StringView&& src, const Placeholder::StringSpecs& specs)
    {
        constexpr auto CountCodepoints = [](StringView& src) {
            if constexpr (std::is_same_v<StringView, StringViewType>) {
                return Util::Unicode::count_codepoints(src.data(), src.size());
            } else {
                return src.codepoints();
            }
        };

        const auto spec_width = Util::Types::to_unsigned(specs.width);
        const auto width = CountCodepoints(src);
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
        constexpr auto FillPattern
            = [](auto& dst, std::basic_string_view<Char> fill, std::size_t fill_len) {
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
            FillPattern(dst, specs.fill, left_padding);
        }

        // Fill data
        using DataChar = typename std::remove_cvref_t<StringView>::value_type;
        if constexpr (std::is_same_v<DataChar, char> && !std::is_same_v<Char, char>) {
            // NOLINTNEXTLINE (cppcoreguidelines-slicing)
            from_multibyte(dst, std::forward<StringView>(src));
        } else {
            dst.append(std::forward<StringView>(src));
        }

        // Fill right padding
        if (right_padding != 0) {
            dst.resize(dst.size() + right_padding * specs.fill.size());
            FillPattern(dst, specs.fill, right_padding);
        }
    }

    std::basic_string<Char> m_pattern;
    std::vector<Placeholder> m_placeholders;
    Levels m_levels;
};

} // namespace PlainCloud::Log
