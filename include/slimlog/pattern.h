/**
 * @file pattern.h
 * @brief Contains the declaration of the Pattern class.
 */

#pragma once

#include "slimlog/common.h"
#include "slimlog/format.h"
#include "slimlog/util/os.h"
#include "slimlog/util/string.h"

#include <slimlog_export.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace SlimLog {

/** @cond */
namespace Detail {

template<typename T>
concept IsPair = requires {
    typename T::first_type;
    typename T::second_type;
} && requires(T pair) {
    pair.first;
    pair.second;
};

} // namespace Detail
/** @endcond */

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

    /** @brief Time function type for getting the current time. */
    using TimeFunctionType = std::pair<std::chrono::sys_seconds, std::size_t> (*)();

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
        SLIMLOG_EXPORT auto get(Level level) const -> CachedStringView<Char>;

        /**
         * @brief Sets the name for the specified log level.
         *
         * @param level The log level.
         * @param name The new name for the log level.
         */
        SLIMLOG_EXPORT auto set(Level level, StringViewType name) -> void;

    private:
        CachedString<Char> m_trace = {'T', 'R', 'A', 'C', 'E'}; ///< Trace level
        CachedString<Char> m_debug = {'D', 'E', 'B', 'U', 'G'}; ///< Debug level
        CachedString<Char> m_info = {'I', 'N', 'F', 'O'}; ///< Info level
        CachedString<Char> m_warning = {'W', 'A', 'R', 'N'}; ///< Warning level
        CachedString<Char> m_error = {'E', 'R', 'R', 'O', 'R'}; ///< Error level
        CachedString<Char> m_fatal = {'F', 'A', 'T', 'A', 'L'}; ///< Fatal level
    };

    struct StringFormatter {
        struct StringSpecs {
            /** @brief Field alignment options. */
            enum class Align : std::uint8_t { None, Left, Right, Center };

            std::size_t width = 0; ///< Field width.
            Align align = Align::None; ///< Field alignment.
            StringViewType fill = StringSpecs::DefaultFill.data(); ///< Fill character.

        private:
            static constexpr std::array<Char, 2> DefaultFill{' ', '\0'};
        };

        explicit StringFormatter(StringViewType value = {});

        template<typename BufferType, typename T>
        constexpr void format(BufferType& out, const CachedStringView<T>& data) const
        {
            if (m_has_padding) [[unlikely]] {
                write_string_padded(out, data);
            } else {
                write_string(out, data);
            }
        }

    protected:
        /**
         * @brief Converts a string to a non-negative integer.
         *
         * This function parses a string and converts it to a non-negative integer.
         * @param begin Reference to a pointer to the beginning of the string.
         *              The pointer will be advanced by the number of characters processed.
         * @param end Pointer to the past-the-end character of the string.
         * @param error_value The default value to return in case of a conversion error.
         * @return The parsed integer, or error_value if the conversion fails.
         */
        static constexpr auto parse_nonnegative_int(
            const Char*& begin, const Char* end, int error_value) noexcept -> int;

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
        static constexpr auto parse_align(const Char* begin, const Char* end, StringSpecs& specs)
            -> const Char*;

        /**
         * @brief Writes the source string to the destination buffer.
         *
         * @tparam T Character type of the source string view.
         * @param dst Destination buffer where the string will be written.
         * @param src Source string view to be written.
         */
        template<typename BufferType, typename T>
        constexpr void write_string(BufferType& dst, const CachedStringView<T>& src) const;

        /**
         * @brief Writes the source string to the destination buffer with specific alignment.
         *
         * This function writes the source string to the destination buffer, applying the
         * specified alignment and fill character.
         *
         * @tparam T Character type of the source string view.
         * @param dst Destination buffer where the string will be written.
         * @param src Source string view to be written.
         * @param specs String specifications, including alignment and fill character.
         */
        template<typename BufferType, typename T>
        constexpr void write_string_padded(BufferType& dst, const CachedStringView<T>& src) const;

    private:
        StringSpecs m_specs;
        bool m_has_padding = false;
    };

    struct CategoryFormatter : public StringFormatter {
        using StringFormatter::StringFormatter;
        static constexpr std::array<Char, 8> Name{'c', 'a', 't', 'e', 'g', 'o', 'r', 'y'};

        template<typename BufferType>
        auto format(BufferType& out, const Record<Char>& record) const -> void
        {
            StringFormatter::format(out, record.category);
        }
    };

    struct LevelFormatter : public StringFormatter {
        using StringFormatter::StringFormatter;
        static constexpr std::array<Char, 5> Name{'l', 'e', 'v', 'e', 'l'};
    };

    struct FileFormatter : public StringFormatter {
        using StringFormatter::StringFormatter;
        static constexpr std::array<Char, 4> Name{'f', 'i', 'l', 'e'};

        template<typename BufferType>
        auto format(BufferType& out, const Record<Char>& record) const -> void
        {
            StringFormatter::format(out, record.filename);
        }
    };

    struct FunctionFormatter : public StringFormatter {
        using StringFormatter::StringFormatter;
        static constexpr std::array<Char, 8> Name{'f', 'u', 'n', 'c', 't', 'i', 'o', 'n'};

        template<typename BufferType>
        auto format(BufferType& out, const Record<Char>& record) const -> void
        {
            StringFormatter::format(out, record.function);
        }
    };

    struct MessageFormatter : public StringFormatter {
        using StringFormatter::StringFormatter;
        static constexpr std::array<Char, 7> Name{'m', 'e', 's', 's', 'a', 'g', 'e'};

        template<typename BufferType>
        auto format(BufferType& out, const Record<Char>& record) const -> void
        {
            StringFormatter::format(out, record.message);
        }
    };

    struct LineFormatter : public CachedFormatter<std::size_t, Char> {
        static constexpr std::array<Char, 4> Name{'l', 'i', 'n', 'e'};

        explicit LineFormatter(StringViewType fmt)
            : CachedFormatter<std::size_t, Char>(std::move(fmt))
        {
        }

        template<typename BufferType>
        auto format(BufferType& out, const Record<Char>& record) const -> void
        {
            return CachedFormatter<std::size_t, Char>::format(out, record.line);
        }
    };

    struct ThreadFormatter : public CachedFormatter<std::size_t, Char> {
        using CachedFormatter<std::size_t, Char>::CachedFormatter;
        static constexpr std::array<Char, 6> Name{'t', 'h', 'r', 'e', 'a', 'd'};
    };
    struct TimeFormatter : public CachedFormatter<std::chrono::sys_seconds, Char> {
        using CachedFormatter<std::chrono::sys_seconds, Char>::CachedFormatter;
        static constexpr std::array<Char, 4> Name{'t', 'i', 'm', 'e'};
    };
    struct MsecFormatter : public CachedFormatter<std::size_t, Char> {
        using CachedFormatter<std::size_t, Char>::CachedFormatter;
        static constexpr std::array<Char, 4> Name{'m', 's', 'e', 'c'};
    };
    struct UsecFormatter : public CachedFormatter<std::size_t, Char> {
        using CachedFormatter<std::size_t, Char>::CachedFormatter;
        static constexpr std::array<Char, 4> Name{'u', 's', 'e', 'c'};
    };
    struct NsecFormatter : public CachedFormatter<std::size_t, Char> {
        using CachedFormatter<std::size_t, Char>::CachedFormatter;
        static constexpr std::array<Char, 4> Name{'n', 's', 'e', 'c'};
    };

    // Variant type holding all possible formatter types
    using FormatterVariant = std::variant<
        StringViewType,
        CategoryFormatter,
        LevelFormatter,
        FileFormatter,
        FunctionFormatter,
        MessageFormatter,
        LineFormatter,
        ThreadFormatter,
        TimeFormatter,
        MsecFormatter,
        UsecFormatter,
        NsecFormatter>;

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
        set_levels(std::forward<Args>(args)...);
        compile(pattern);
    }

    /**
     * @brief Checks if the pattern is empty.
     *
     * @return \b true if the pattern is an empty string.
     * @return \b false if the pattern is not an empty string.
     */
    [[nodiscard]] SLIMLOG_EXPORT auto empty() const -> bool;

    /**
     * @brief Formats a message according to the pattern.
     *
     * This function formats a log message based on the specified pattern.
     *
     * @tparam BufferType Buffer type for the format output.
     * @param out Buffer storing the raw message to be overwritten with the result.
     * @param record Log record.
     */
    template<typename BufferType>
    SLIMLOG_EXPORT auto format(BufferType& out, const Record<Char>& record) -> void;

    /**
     * @brief Sets the time function used for log timestamps.
     *
     * @param time_func Time function to be set for this pattern.
     */
    SLIMLOG_EXPORT auto set_time_func(TimeFunctionType time_func) -> void;

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
    SLIMLOG_EXPORT auto set_pattern(StringViewType pattern) -> void;

    /**
     * @brief Sets the log level names with containers.
     *
     * Usage example:
     * ```cpp
     * std::vector<std::pair<Level, std::string_view>> levels = {{Level::Info, "INFO"}};
     * pattern.set_levels(levels);
     * ```
     */
    template<typename Container>
        requires(!Detail::IsPair<std::remove_cvref_t<Container>>)
    auto set_levels(Container&& container) -> void
    {
        for (const auto& pair : std::forward<Container>(container)) {
            m_levels.set(pair.first, StringViewType{pair.second});
        }
    }

    /**
     * @brief Sets the log level names with variadic arguments.
     * @overload
     *
     * Usage example:
     * ```cpp
     * pattern.set_levels(
     *     std::make_pair(Level::Info, from_utf8<Char>("CUSTOM_INFO")),
     *     std::make_pair(Level::Debug, from_utf8<Char>("CUSTOM_DEBUG"))
     * );
     * ```
     *
     * @param pairs Variadic list of level-name pairs.
     */
    template<typename... Pairs>
        requires(Detail::IsPair<std::remove_cvref_t<Pairs>> && ...)
    auto set_levels(Pairs&&... pairs) -> void
    {
        (
            [&]() {
                auto&& pair = std::forward<Pairs>(pairs);
                m_levels.set(pair.first, StringViewType{pair.second});
            }(),
            ...);
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
    SLIMLOG_EXPORT void compile(StringViewType pattern);

private:
    /**
     * @brief Append a pattern placeholder to the list of placeholders.
     *
     * This function creates the appropriate wrapper formatter and appends it to the list.
     *
     * @param field_type Field type identifier.
     * @param count Placeholder length.
     * @param shift Margin from the end of the pattern string.
     */
    template<typename PlaceholderType>
    void append_placeholder(std::size_t count);

    /**
     * @brief Append raw text placeholder.
     *
     * @param count Text length.
     * @param shift Margin from the end of the pattern string.
     */
    void append_text(std::size_t count, std::size_t shift = 0);

    std::basic_string<Char> m_pattern;
    std::vector<FormatterVariant> m_placeholders;
    Levels m_levels;
    TimeFunctionType m_time_func = Util::OS::local_time;
    bool m_has_time = false;
};

} // namespace SlimLog

#ifdef SLIMLOG_HEADER_ONLY
#include "slimlog/pattern-inl.h" // IWYU pragma: keep
#endif
