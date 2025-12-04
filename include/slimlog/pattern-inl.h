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
#include <variant>

namespace SlimLog {

template<typename Char>
auto Pattern<Char>::Levels::get(Level level) -> RecordStringView<Char>&
{
    switch (level) {
    case Level::Fatal:
        return m_fatal_sv;
    case Level::Error:
        return m_error_sv;
    case Level::Warning:
        return m_warning_sv;
    case Level::Info:
        return m_info_sv;
    case Level::Debug:
        return m_debug_sv;
    case Level::Trace:
        [[fallthrough]];
    default:
        return m_trace_sv;
    }
}

template<typename Char>
auto Pattern<Char>::Levels::set(Level level, StringViewType name) -> void
{
    switch (level) {
    case Level::Fatal:
        m_fatal = std::move(name);
        m_fatal_sv = RecordStringView<Char>{m_fatal};
        break;
    case Level::Error:
        m_error = std::move(name);
        m_error_sv = RecordStringView<Char>{m_error};
        break;
    case Level::Warning:
        m_warning = std::move(name);
        m_warning_sv = RecordStringView<Char>{m_warning};
        break;
    case Level::Info:
        m_info = std::move(name);
        m_info_sv = RecordStringView<Char>{m_info};
        break;
    case Level::Debug:
        m_debug = std::move(name);
        m_debug_sv = RecordStringView<Char>{m_debug};
        break;
    case Level::Trace:
        m_trace = std::move(name);
        m_trace_sv = RecordStringView<Char>{m_trace};
        break;
    }
}

template<typename Char>
[[nodiscard]] auto Pattern<Char>::empty() const -> bool
{
    return m_pattern.empty();
}

template<typename Char>
auto Pattern<Char>::format(auto& out, const Record<Char>& record) -> void
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
            format_string(out, item.value, record.filename);
            break;
        case Placeholder::Type::Function:
            format_string(out, item.value, record.function);
            break;
        case Placeholder::Type::Line:
            format_generic(out, item.value, record.line);
            break;
        case Placeholder::Type::Time:
            format_generic(out, item.value, record.time.first);
            break;
        case Placeholder::Type::Msec:
            format_generic(out, item.value, record.time.second / MsecInNsec);
            break;
        case Placeholder::Type::Usec:
            format_generic(out, item.value, record.time.second / UsecInNsec);
            break;
        case Placeholder::Type::Nsec:
            format_generic(out, item.value, record.time.second);
            break;
        case Placeholder::Type::Thread:
            format_generic(out, item.value, record.thread_id);
            break;
        case Placeholder::Type::Message:
            format_string(out, item.value, record.message);
            break;
        }
    }
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
                Placeholders::List.begin(), Placeholders::List.end(), [pattern](const auto& item) {
                    return pattern.starts_with(item.second);
                });
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

template<typename Char>
template<typename StringView>
    requires(
        std::same_as<std::remove_cvref_t<StringView>, RecordStringView<Char>>
        || std::same_as<std::remove_cvref_t<StringView>, RecordStringView<char>>)
void Pattern<Char>::format_string(auto& out, const auto& item, StringView&& data)
{
    if (auto& specs = std::get<typename Placeholder::StringSpecs>(item); specs.width > 0)
        [[unlikely]] {
        write_string_padded(out, std::forward<StringView>(data), specs);
    } else {
        write_string(out, std::forward<StringView>(data));
    }
}

template<typename Char>
template<typename T>
void Pattern<Char>::format_generic(auto& out, const auto& item, T data)
{
    std::get<CachedFormatter<T, Char>>(item).format(out, data);
}

template<typename Char>
constexpr auto Pattern<Char>::parse_nonnegative_int(
    const Char*& begin, const Char* end, int error_value) noexcept -> int
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
            && prev * Base + static_cast<unsigned>(ptr[-1] - '0') <= MaxInt
        ? static_cast<int>(value)
        : error_value;
}

template<typename Char>
constexpr auto Pattern<Char>::parse_align(
    const Char* begin, const Char* end, Placeholder::StringSpecs& specs) -> const Char*
{
    auto align = Placeholder::StringSpecs::Align::None;
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
void Pattern<Char>::append_placeholder(Placeholder::Type type, std::size_t count, std::size_t shift)
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
        m_placeholders.emplace_back(type, CachedFormatter<std::chrono::sys_seconds, Char>(data));
        break;
    }
};

template<typename Char>
auto Pattern<Char>::get_string_specs(StringViewType value) -> Placeholder::StringSpecs
{
    typename Placeholder::StringSpecs specs = {};
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
    return specs;
}

template<typename Char>
template<typename StringView>
constexpr void Pattern<Char>::write_string(auto& dst, StringView&& src)
{
    using DataChar = typename std::remove_cvref_t<StringView>::value_type;
    if constexpr (std::is_same_v<DataChar, char> && !std::is_same_v<Char, char>) {
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
        dst.append(std::forward<StringView>(src));
    }
}

template<typename Char>
template<typename StringView>
constexpr void Pattern<Char>::write_string_padded(
    auto& dst, StringView&& src, const typename Placeholder::StringSpecs& specs)
{
    const auto spec_width = Util::Types::to_unsigned(specs.width);
    const auto codepoints = src.codepoints();
    const auto padding = spec_width > codepoints ? spec_width - codepoints : 0;

    // Shifts are encoded as string literals because
    // static is not supported in constexpr functions.
    const char* shifts = "\x1f\x1f\x00\x01";
    const auto left_padding
        = padding >> static_cast<unsigned>(shifts[static_cast<int>(specs.align)]);
    const auto right_padding = padding - left_padding;

    // Reserve exact amount for data + padding upfront
    const auto fill_size = specs.fill.size();
    const auto fill_data = specs.fill.data();
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
    write_string(dst, std::forward<StringView>(src));

    // Fill right padding
    if (right_padding > 0) {
        FastFill(dst, fill_data, fill_size, right_padding);
    }
}

} // namespace SlimLog
