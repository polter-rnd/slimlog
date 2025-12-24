/**
 * @file pattern-inl.h
 * @brief Contains the definition of the Pattern class.
 */

#pragma once

// IWYU pragma: private, include "slimlog/pattern.h"

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "slimlog/pattern.h" // IWYU pragma: associated
#include "slimlog/util/types.h"
#include "slimlog/util/unicode.h"

#include <algorithm>
#include <climits>
#include <limits>

namespace SlimLog {

template<typename Char>
auto Pattern<Char>::Levels::get(Level level) const -> CachedStringView<Char>
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

template<typename Char>
auto Pattern<Char>::Levels::set(Level level, StringViewType name) -> void
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
        [[fallthrough]];
    default:
        m_trace = std::move(name);
        break;
    }
}

template<typename Char>
[[nodiscard]] auto Pattern<Char>::empty() const -> bool
{
    return m_pattern.empty();
}

template<typename Char>
template<typename BufferType>
auto Pattern<Char>::format(BufferType& out, const Record<Char>& record) -> void
{
    std::pair<std::chrono::sys_seconds, std::size_t> time_point;
    if (m_has_time) {
        time_point = m_time_func();
    }

    for (const auto& item : m_placeholders) {
        std::visit(
            Util::Types::Overloaded{
                [&out](const StringViewType& text) { out.append(text); },
                [&out, &record, this](const LevelFormatter& formatter) {
                    formatter.format(out, m_levels.get(record.level));
                },
                [&out, &time_point](const TimeFormatter& formatter) {
                    formatter.format(out, time_point.first);
                },
                [&out, &time_point](const MsecFormatter& formatter) {
                    constexpr std::size_t MsecInNsec = 1000000;
                    formatter.format(out, time_point.second / MsecInNsec);
                },
                [&out, &time_point](const UsecFormatter& formatter) {
                    constexpr std::size_t UsecInNsec = 1000;
                    formatter.format(out, time_point.second / UsecInNsec);
                },
                [&out, &time_point](const NsecFormatter& formatter) {
                    formatter.format(out, time_point.second);
                },
                [&out](const ThreadFormatter& formatter) {
                    formatter.format(out, Util::OS::thread_id());
                },
                [&out, &record](const auto& formatter) { formatter.format(out, record); }},
            item);
    }
}

template<typename Char>
auto Pattern<Char>::set_time_func(TimeFunctionType time_func) -> void
{
    m_time_func = time_func;
}

template<typename Char>
auto Pattern<Char>::set_pattern(StringViewType pattern) -> void
{
    compile(pattern);
}

template<typename Char>
void Pattern<Char>::compile(StringViewType pattern)
{
    m_placeholders.clear();
    m_pattern.clear();
    m_pattern.reserve(pattern.size());

    bool inside_placeholder = false;
    for (;;) {
        constexpr std::array<Char, 3> Delimeters{'{', '}', '\0'};
        const auto pos = pattern.find_first_of(Delimeters.data());
        const auto len = pattern.size();

        if (pos == pattern.npos) {
            // Append leftovers, if any
            if (inside_placeholder) {
                throw FormatError("format error: unmatched '{' in pattern string");
            }
            if (!pattern.empty()) {
                m_pattern.append(pattern);
                append_text(len);
            }
            break;
        }

        const auto chr = Util::Unicode::to_ascii(pattern[pos]);

        // Handle escaped braces
        if (!inside_placeholder && pos < len - 1
            && chr == Util::Unicode::to_ascii(pattern[pos + 1])) {
            m_pattern.append(pattern.substr(0, pos + 1));
            pattern = pattern.substr(pos + 2);
            append_text(pos + 1);
            continue;
        }

        if (!inside_placeholder && chr == '{') {
            // Enter inside placeholder
            m_pattern.append(pattern.substr(0, pos + 1));
            pattern = pattern.substr(pos + 1);
            append_text(pos, 1);

            inside_placeholder = true;
        } else if (inside_placeholder && chr == '}') {
            std::size_t delta = 0;
            auto find_placeholder = [&delta, &pattern, &pos, this]<typename PlaceholderType>() {
                if constexpr (!std::is_same_v<PlaceholderType, StringViewType>) {
                    const StringViewType placeholder{
                        PlaceholderType::Name.data(), PlaceholderType::Name.size()};
                    if (delta == 0 && pattern.starts_with(placeholder)) {
                        delta = placeholder.size();
                        m_pattern.append(pattern.substr(delta, pos + 1 - delta));
                        pattern = pattern.substr(pos + 1);
                        append_placeholder<PlaceholderType>(pos - delta);
                    }
                }
            };
            Util::Types::variant_for_each_type<FormatterVariant>(find_placeholder);

            if (delta == 0) {
                throw FormatError("format error: unknown pattern placeholder found");
            }

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
        m_placeholders.emplace_back(MessageFormatter{});
    }
}

template<typename Char>
void Pattern<Char>::append_text(std::size_t count, std::size_t shift)
{
    auto data = StringViewType{m_pattern}.substr(m_pattern.size() - count - shift, count);

    // Try to merge with previous text placeholder
    if (!m_placeholders.empty()) {
        if (auto* text = std::get_if<StringViewType>(&m_placeholders.back())) {
            // Merge by extending the string view
            *text = StringViewType({text->data(), text->size() + data.size()});
            return;
        }
    }

    // Otherwise add new text placeholder
    m_placeholders.emplace_back(data);
}

template<typename Char>
template<typename PlaceholderType>
void Pattern<Char>::append_placeholder(std::size_t count)
{
    auto data = StringViewType{m_pattern}.substr(m_pattern.size() - count, count);
    m_placeholders.emplace_back(PlaceholderType{data});
    if constexpr (
        std::is_same_v<PlaceholderType, TimeFormatter>
        || std::is_same_v<PlaceholderType, MsecFormatter>
        || std::is_same_v<PlaceholderType, UsecFormatter>
        || std::is_same_v<PlaceholderType, NsecFormatter>) {
        m_has_time = true;
    }
}

template<typename Char>
Pattern<Char>::StringFormatter::StringFormatter(StringViewType value)
{
    StringSpecs specs = {};
    if (!value.empty()) {
        const auto* begin = value.data();
        const auto* end = begin + value.size();
        const auto* fmt = parse_align(begin, end, specs);
        if (auto chr = Util::Unicode::to_ascii(*fmt); chr != '}') {
            const int width = parse_nonnegative_int(fmt, end - 1, -1);
            if (width == -1) {
                throw FormatError("format field width is too big");
            }
            chr = Util::Unicode::to_ascii(*fmt);
            switch (chr) {
            case '}':
                break;
            case 's':
                if (Util::Unicode::to_ascii(fmt[1]) != '}') {
                    throw FormatError("missing '}' in format string");
                }
                break;
            default:
                throw FormatError("wrong format type");
            }
            specs.width = width;
        }
    }
    m_specs = specs;
    m_has_padding = specs.width > 0;
}

template<typename Char>
constexpr auto Pattern<Char>::StringFormatter::parse_nonnegative_int(
    const Char*& begin, const Char* end, int error_value) noexcept -> int
{
    unsigned value = 0;
    unsigned prev = 0;
    auto ptr = begin;
    constexpr auto Base = 10U;
    while (ptr != end && '0' <= *ptr && *ptr <= '9') {
        prev = value;
        value = (value * Base) + static_cast<unsigned>(*ptr - '0');
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
            && (prev * Base) + static_cast<unsigned>(ptr[-1] - '0') <= MaxInt
        ? static_cast<int>(value)
        : error_value;
}

template<typename Char>
constexpr auto Pattern<Char>::StringFormatter::parse_align(
    const Char* begin, const Char* end, StringSpecs& specs) -> const Char*
{
    auto align = StringSpecs::Align::None;
    auto* ptr = begin + Util::Unicode::code_point_length(begin);
    if (end - ptr <= 0) {
        // Can happen if invalid code point claims more bytes than available
        // in the string, so we just reset pointer to the beginning
        // to avoid dereferencing out of bounds.
        ptr = begin;
    }

    for (;;) {
        switch (Util::Unicode::to_ascii(*ptr)) {
        case '<':
            align = StringSpecs::Align::Left;
            break;
        case '>':
            align = StringSpecs::Align::Right;
            break;
        case '^':
            align = StringSpecs::Align::Center;
            break;
        }
        if (align != StringSpecs::Align::None) {
            if (ptr != begin) {
                specs.fill = StringViewType(begin, Util::Types::to_unsigned(ptr - begin));
                begin = ptr + 1;
            } else {
                ++begin;
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

template<typename Char>
template<typename BufferType, typename T>
constexpr void Pattern<Char>::StringFormatter::write_string(
    BufferType& dst, const CachedStringView<T>& src) const
{
    if constexpr (std::is_same_v<T, char> && !std::is_same_v<Char, char>) {
        // Calculate destination buffer size based on target character encoding:
        // - UTF-8 (1 byte): same size as source (byte-for-byte copy)
        // - UTF-16 (2 bytes): double codepoints (potential surrogate pairs)
        // - UTF-32 (4 bytes): same as codepoints (one-to-one mapping)
        const auto dest_size
            = sizeof(Char) == 1 ? src.size() : src.codepoints() * (sizeof(Char) == 2 ? 2 : 1);
        if (dest_size == 0) {
            return;
        }

        dst.reserve(dst.size() + dest_size);
        const auto written = Util::Unicode::from_utf8(dst.end(), dest_size, src.data(), src.size());
        dst.resize(dst.size() + written);
    } else {
        dst.append(src);
    }
}

template<typename Char>
template<typename BufferType, typename T>
constexpr void Pattern<Char>::StringFormatter::write_string_padded(
    BufferType& dst, const CachedStringView<T>& src) const
{
    const auto spec_width = Util::Types::to_unsigned(m_specs.width);
    const auto codepoints = src.codepoints();
    const auto padding = spec_width > codepoints ? spec_width - codepoints : 0;

    // Shifts are encoded as string literals because
    // static is not supported in constexpr functions.
    const char* shifts = "\x1f\x1f\x00\x01";
    const auto left_padding
        = padding >> static_cast<unsigned>(shifts[static_cast<int>(m_specs.align)]);
    const auto right_padding = padding - left_padding;

    // Reserve exact amount for data + padding upfront
    const auto fill_size = m_specs.fill.size();
    const auto fill_data = m_specs.fill.data();
    dst.reserve(dst.size() + codepoints + (padding * fill_size));

    // Highly optimized fill function using large chunks
    constexpr auto FastFill = [](auto& out, const Char* src, std::size_t src_len, std::size_t cnt) {
        const auto total_chars = cnt * src_len;
        const auto start_pos = out.size();
        out.resize(start_pos + total_chars);
        auto* dst = out.data() + start_pos;

        if (src_len == 1 && sizeof(Char) == 1) {
            // Single character - fastest way to fill
            std::fill_n(dst, total_chars, src[0]);
            return;
        }

        // For multi-byte single chars and multi-character patterns
        constexpr std::size_t ChunkSize = 65536; // 64KB
        if (src_len == 1) {
            // Fill first chunk, then copy in large blocks
            std::fill_n(dst, std::min(total_chars, ChunkSize), src[0]);

            for (std::size_t pos = ChunkSize; pos < total_chars; pos += ChunkSize) {
                const auto size = std::min(ChunkSize, total_chars - pos);
                std::copy_n(dst, size, dst + pos);
            }
        } else {
            // Multi-character pattern - exponential doubling up to 64KB
            std::copy_n(src, src_len, dst);
            for (std::size_t current = src_len; current < total_chars;) {
                const auto size = std::min({current, ChunkSize, total_chars - current});
                std::copy_n(dst, size, dst + current);
                current += size;
            }
        }
    };

    // Fill left padding
    if (left_padding > 0) {
        FastFill(dst, fill_data, fill_size, left_padding);
    }

    // Fill data
    write_string(dst, src);

    // Fill right padding
    if (right_padding > 0) {
        FastFill(dst, fill_data, fill_size, right_padding);
    }
}

} // namespace SlimLog
