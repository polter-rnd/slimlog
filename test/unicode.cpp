#include "slimlog/util/unicode.h"

#include <mettle.hpp>

#include <array>
#include <cstdint>
#include <string>

// IWYU pragma: no_include <functional>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace slimlog::util::unicode;

// Constexpr test helpers - these will only compile if the functions are truly constexpr
constexpr auto test_constexpr_code_point_length() -> bool
{
    // Test with ASCII characters
    constexpr char Ascii[] = "Hello";
    static_assert(code_point_length(Ascii) == 1);
    static_assert(code_point_length(Ascii + 1) == 1);

    // Test with UTF-8 multi-byte characters
    constexpr char8_t Utf8Cyrillic[] = u8"\u041F\u0440"; // "Пр"
    static_assert(code_point_length(Utf8Cyrillic) == 2); // 2-byte character
    static_assert(code_point_length(Utf8Cyrillic + 2) == 2);

    constexpr char8_t Utf8Chinese[] = u8"\u4F60\u597D"; // "你好"
    static_assert(code_point_length(Utf8Chinese) == 3); // 3-byte character
    static_assert(code_point_length(Utf8Chinese + 3) == 3);

    constexpr char8_t Utf8Emoji[] = u8"\U0001F600"; // "😀"
    static_assert(code_point_length(Utf8Emoji) == 4); // 4-byte character

    // Test with wide characters
    constexpr wchar_t Wide[] = L"Hello";
    static_assert(code_point_length(Wide) == 1);

    // Test UTF-16 BMP characters
    constexpr char16_t Utf16[] = u"Hello";
    static_assert(code_point_length(Utf16) == 1);

    // Test UTF-16 with surrogate pairs
    constexpr char16_t Utf16Emoji[] = u"\U0001F600\U0001F601"; // "😀😁"
    static_assert(code_point_length(Utf16Emoji) == 2); // Surrogate pair
    static_assert(code_point_length(Utf16Emoji + 2) == 2); // Next surrogate pair

    // Test UTF-16 mixed BMP and surrogate pairs
    constexpr char16_t Utf16Mixed[] = u"A\U0001F600"; // "A😀"
    static_assert(code_point_length(Utf16Mixed) == 1); // 'A' is BMP
    static_assert(code_point_length(Utf16Mixed + 1) == 2); // Emoji is surrogate pair

    // Test UTF-32 character (should always return 1)
    constexpr char32_t Utf32Emoji[] = U"\U0001F600"; // "😀"
    static_assert(code_point_length(Utf32Emoji) == 1);

    return true;
}

constexpr auto test_constexpr_count_codepoints() -> bool
{
    // Test with ASCII
    constexpr char Ascii[] = "Hello";
    static_assert(count_codepoints(Ascii, 5) == 5);

    // Test with wide characters
    constexpr wchar_t Wide[] = L"Hello";
    static_assert(count_codepoints(Wide, 5) == 5);

    constexpr char16_t Utf16[] = u"Hello";
    static_assert(count_codepoints(Utf16, 5) == 5);

    constexpr char32_t Utf32[] = U"Hello";
    static_assert(count_codepoints(Utf32, 5) == 5);

    return true;
}

constexpr auto test_constexpr_to_ascii() -> bool
{
    static_assert(to_ascii('A') == 'A');
    static_assert(to_ascii('z') == 'z');
    static_assert(to_ascii('0') == '0');
    static_assert(to_ascii(' ') == ' ');
    static_assert(to_ascii(127) == static_cast<char>(127));
    static_assert(to_ascii(255) == static_cast<char>(255));
    static_assert(to_ascii(256) == '\0');
    static_assert(to_ascii(1000) == '\0');
    static_assert(to_ascii(0x1F600) == '\0');
    return true;
}

// Runtime test suite for Unicode utilities
const suite<> Unicode("unicode", [](auto& _) {
    // Compile-time verification
    static_assert(test_constexpr_code_point_length());
    static_assert(test_constexpr_count_codepoints());
    static_assert(test_constexpr_to_ascii());

    // Test code_point_length function
    _.test("code_point_length", []() {
        // ASCII characters (1 byte)
        const char ascii[] = "Hello";
        expect(code_point_length(ascii), equal_to(1));
        expect(code_point_length(ascii + 1), equal_to(1));

        // UTF-8 2-byte characters (Cyrillic)
        const char8_t cyrillic[] = u8"\u041F\u0440\u0438\u0432\u0435\u0442"; // "Привет"
        expect(code_point_length(cyrillic), equal_to(2)); // П
        expect(code_point_length(cyrillic + 2), equal_to(2)); // р

        // UTF-8 3-byte characters (Chinese)
        const char8_t chinese[] = u8"\u4F60\u597D"; // "你好"
        expect(code_point_length(chinese), equal_to(3)); // 你
        expect(code_point_length(chinese + 3), equal_to(3)); // 好

        // UTF-8 4-byte characters (emojis)
        const char8_t emoji[] = u8"\U0001F600\U0001F601"; // "😀😁"
        expect(code_point_length(emoji), equal_to(4)); // 😀
        expect(code_point_length(emoji + 4), equal_to(4)); // 😁

        // Test UTF-16 with surrogate pairs (emojis and supplementary characters)
        // U+1F600 (😀) in UTF-16 becomes surrogate pair: 0xD83D 0xDE00
        const char16_t utf16_emoji[] = u"\U0001F600\U0001F601"; // "😀😁"
        expect(code_point_length(utf16_emoji), equal_to(2)); // High surrogate indicates pair
        expect(code_point_length(utf16_emoji + 2), equal_to(2)); // Next emoji also a pair

        // Test UTF-16 BMP character followed by surrogate pair
        const char16_t utf16_mixed[] = u"A\U0001F600"; // "A😀"
        expect(code_point_length(utf16_mixed), equal_to(1)); // 'A' is BMP, 1 code unit
        expect(code_point_length(utf16_mixed + 1), equal_to(2)); // Emoji is surrogate pair

        // Test wide characters
        if constexpr (sizeof(wchar_t) == 2) {
            const wchar_t wchar_emoji[] = L"\U0001F600"; // "😀"
            expect(code_point_length(wchar_emoji), equal_to(2)); // Surrogate pair
        } else if constexpr (sizeof(wchar_t) == 4) {
            expect(code_point_length(static_cast<wchar_t*>(nullptr)), equal_to(1)); // Always 1
        }

        // Test UTF-32 character (should always return 1)
        expect(code_point_length(static_cast<char32_t*>(nullptr)), equal_to(1));
    });

    // Test utf8_decode function
    _.test("utf8_decode", []() {
        std::uint8_t state = 0;
        std::uint32_t codepoint = 0;

        // Test ASCII character 'A' (0x41)
        state = 0;
        expect(utf8_decode(state, codepoint, 0x41), equal_to(0));
        expect(codepoint, equal_to(0x41U));

        // Test 2-byte UTF-8 sequence: 'П' (U+041F -> 0xD0 0x9F)
        state = 0;
        expect(utf8_decode(state, codepoint, 0xD0), greater(1));
        expect(utf8_decode(state, codepoint, 0x9F), equal_to(0));
        expect(codepoint, equal_to(0x041FU));

        // Test 3-byte UTF-8 sequence: '你' (U+4F60 -> 0xE4 0xBD 0xA0)
        state = 0;
        expect(utf8_decode(state, codepoint, 0xE4), greater(1));
        expect(utf8_decode(state, codepoint, 0xBD), greater(1));
        expect(utf8_decode(state, codepoint, 0xA0), equal_to(0));
        expect(codepoint, equal_to(0x4F60U));

        // Test 4-byte UTF-8 sequence: '😀' (U+1F600 -> 0xF0 0x9F 0x98 0x80)
        state = 0;
        expect(utf8_decode(state, codepoint, 0xF0), greater(1));
        expect(utf8_decode(state, codepoint, 0x9F), greater(1));
        expect(utf8_decode(state, codepoint, 0x98), greater(1));
        expect(utf8_decode(state, codepoint, 0x80), equal_to(0));
        expect(codepoint, equal_to(0x1F600U));

        // Test invalid UTF-8 sequence
        state = 0;
        expect(utf8_decode(state, codepoint, 0xFF), equal_to(1)); // Invalid byte
    });

    // Test count_codepoints function
    _.test("count_codepoints", []() {
        // ASCII string
        const char ascii[] = "Hello";
        expect(count_codepoints(ascii, 5), equal_to(5U));

        // Mixed UTF-8 string: "Hello, 世界!"
        const std::u8string mixed = u8"Hello, \u4E16\u754C!";
        expect(
            count_codepoints(mixed.data(), mixed.size()),
            equal_to(10U)); // 7 ASCII + 2 Chinese + 1 ASCII

        // Pure Cyrillic: "Привет"
        const std::u8string cyrillic = u8"\u041F\u0440\u0438\u0432\u0435\u0442";
        expect(count_codepoints(cyrillic.data(), cyrillic.size()), equal_to(6U));

        // Emojis: "😀😁😂"
        const std::u8string emojis = u8"\U0001F600\U0001F601\U0001F602";
        expect(count_codepoints(emojis.data(), emojis.size()), equal_to(3U));

        // Empty string
        expect(count_codepoints("", 0), equal_to(0U));

        // Wide character strings (should return length as-is)
        const std::wstring wide = L"Hello";
        expect(count_codepoints(wide.data(), wide.size()), equal_to(5U));

        // Invalid UTF-8 sequence
        const std::string invalid = "\xFF\xFE\xFD";
        expect(
            count_codepoints(invalid.data(), invalid.size()),
            equal_to(0U)); // Should stop at first invalid byte
    });

    // Test to_ascii function
    _.test("to_ascii", []() {
        // Valid ASCII range
        expect(to_ascii('A'), equal_to('A'));
        expect(to_ascii('z'), equal_to('z'));
        expect(to_ascii('0'), equal_to('0'));
        expect(to_ascii(' '), equal_to(' '));
        expect(to_ascii('\n'), equal_to('\n'));
        expect(to_ascii(127), equal_to(static_cast<char>(127)));
        expect(to_ascii(255), equal_to(static_cast<char>(255)));

        // Out of ASCII range
        expect(to_ascii(256), equal_to('\0'));
        expect(to_ascii(1000), equal_to('\0'));
        expect(to_ascii(0x1F600), equal_to('\0')); // Emoji codepoint

        // Test with different character types
        expect(to_ascii(static_cast<char16_t>('A')), equal_to('A'));
        expect(to_ascii(static_cast<char32_t>('B')), equal_to('B'));
        expect(to_ascii(static_cast<wchar_t>('C')), equal_to('C'));
    });

    // Test write_codepoint function
    _.test("write_codepoint", []() {
        // Test with char32_t (direct codepoint assignment)
        {
            std::array<char32_t, 10> buffer{};

            // ASCII character
            auto written = write_codepoint(buffer.data(), buffer.size(), 0x41); // 'A'
            expect(written, equal_to(1U));
            expect(buffer[0], equal_to(U'A'));

            // Unicode character
            written = write_codepoint(buffer.data(), buffer.size(), 0x4F60); // '你'
            expect(written, equal_to(1U));
            expect(buffer[0], equal_to(U'\u4F60'));

            // Emoji
            written = write_codepoint(buffer.data(), buffer.size(), 0x1F600); // '😀'
            expect(written, equal_to(1U));
            expect(buffer[0], equal_to(U'\U0001F600'));
        }

        // Test with char16_t (surrogate pairs for high codepoints)
        {
            std::array<char16_t, 10> buffer{};

            // BMP character (no surrogate needed)
            auto written = write_codepoint(buffer.data(), buffer.size(), 0x4F60); // '你'
            expect(written, equal_to(1U));
            expect(buffer[0], equal_to(u'\u4F60'));

            // Supplementary plane character (needs surrogate pair)
            written = write_codepoint(buffer.data(), buffer.size(), 0x1F600); // '😀'
            expect(written, equal_to(2U));
            expect(buffer[0], equal_to(char16_t{0xD83D})); // High surrogate
            expect(buffer[1], equal_to(char16_t{0xDE00})); // Low surrogate
        }

        // Test edge cases
        {
            // Buffer too small - try write a codepoint that requires surrogate pair
            std::array<char16_t, 1> small_buffer{};
            auto written = write_codepoint(small_buffer.data(), small_buffer.size(), 0x1f600);
            expect(written, equal_to(0U));

            // Buffer is empty
            written = write_codepoint(small_buffer.data(), 0, 0x41);
            expect(written, equal_to(0U));

            // Null pointer
            written = write_codepoint(static_cast<char32_t*>(nullptr), 10, 0x41);
            expect(written, equal_to(0U));
        }
    });

    // Test from_utf8 function
    _.test("from_utf8", []() {
        // Test conversion to char (should be direct copy)
        {
            const std::u8string source = u8"Hello, \u4E16\u754C!"; // "Hello, 世界!"
            std::array<char, 20> dest{};

            auto written = from_utf8(dest.data(), dest.size(), source.data(), source.size());
            expect(written, equal_to(source.size()));
            for (std::size_t i = 0; i < written; ++i) {
                expect(static_cast<char>(source[i]), equal_to(dest.at(i)));
            }
        }

        // Test conversion to char32_t
        {
            const std::u8string source = u8"A\u4F60\U0001F600"; // "A你😀"
            std::array<char32_t, 10> dest{};

            auto written = from_utf8(dest.data(), dest.size(), source.data(), source.size());
            expect(written, equal_to(3U)); // 3 codepoints
            expect(dest[0], equal_to(U'A'));
            expect(dest[1], equal_to(U'\u4F60'));
            expect(dest[2], equal_to(U'\U0001F600'));
        }

        // Test conversion to char16_t (with surrogate pairs)
        {
            const std::u8string source = u8"A\U0001F600"; // "A😀"
            std::array<char16_t, 10> dest{};

            auto written = from_utf8(dest.data(), dest.size(), source.data(), source.size());
            expect(written, equal_to(3U)); // 'A' + surrogate pair for '😀'
            expect(dest[0], equal_to(u'A'));
            expect(dest[1], equal_to(char16_t{0xD83D})); // High surrogate
            expect(dest[2], equal_to(char16_t{0xDE00})); // Low surrogate
        }

        // Test edge cases
        {
            std::array<char32_t, 10> dest{};

            // Empty source
            auto written = from_utf8(dest.data(), dest.size(), "", 0);
            expect(written, equal_to(0U));

            // Null source
            written = from_utf8(dest.data(), dest.size(), static_cast<char*>(nullptr), 10);
            expect(written, equal_to(0U));

            // Null dest
            written = from_utf8(static_cast<char32_t*>(nullptr), 10, "test", 4);
            expect(written, equal_to(0U));

            // Zero dest size
            written = from_utf8(dest.data(), 0, "test", 4);
            expect(written, equal_to(0U));
        }

        // Test buffer size limits
        {
            const std::string source = "Hello";
            std::array<char32_t, 3> small_dest{};

            auto written
                = from_utf8(small_dest.data(), small_dest.size(), source.data(), source.size());
            expect(written, equal_to(3U)); // Should stop when buffer is full
            expect(small_dest[0], equal_to(U'H'));
            expect(small_dest[1], equal_to(U'e'));
            expect(small_dest[2], equal_to(U'l'));
        }

        // Test invalid UTF-8 sequences
        {
            const std::string invalid = "\xFF\x41\x42"; // Invalid byte followed by valid ones
            std::array<char32_t, 10> dest{};

            auto written = from_utf8(dest.data(), dest.size(), invalid.data(), invalid.size());
            expect(written, equal_to(0U)); // Should stop at invalid sequence
        }

        // Test incomplete UTF-8 sequences
        {
            const std::string incomplete
                = "\x41\x42\xE4\xBD"; // AB followed by incomplete 3-byte sequence for '你'
            std::array<char32_t, 10> dest{};

            auto written
                = from_utf8(dest.data(), dest.size(), incomplete.data(), incomplete.size());
            expect(written, equal_to(2)); // Only 'A' and 'B' should be written
            expect(dest[0], equal_to(U'A'));
            expect(dest[1], equal_to(U'B'));
        }
    });

    // Test from_utf8 overload with single string argument
    _.test("from_utf8_string", []() {
        // Test normal behavior with string literal
        {
            const auto result = from_utf8<char32_t>("Hello");
            expect(result.size(), equal_to(5U));
            expect(result, equal_to(U"Hello"));
        }

        // Test with std::string
        {
            const std::string source = "World";
            auto result = from_utf8<char32_t>(source);
            expect(result.size(), equal_to(5U));
            expect(result, equal_to(U"World"));
        }

        // Test with std::u8string
        {
            const std::u8string source = u8"Test";
            auto result = from_utf8<char32_t>(source);
            expect(result.size(), equal_to(4U));
            expect(result, equal_to(U"Test"));
        }

        // Test with multi-byte UTF-8 characters
        {
            auto result = from_utf8<char32_t>("A\u4F60\U0001F600"); // "A你😀"
            expect(result.size(), equal_to(3U));
            expect(result[0], equal_to(U'A'));
            expect(result[1], equal_to(U'\u4F60'));
            expect(result[2], equal_to(U'\U0001F600'));
        }

        // Test conversion to char16_t with surrogate pairs
        {
            auto result = from_utf8<char16_t>("A\U0001F600"); // "A😀"
            expect(result.size(), equal_to(3U)); // 'A' + surrogate pair
            expect(result[0], equal_to(u'A'));
            expect(result[1], equal_to(char16_t{0xD83D})); // High surrogate
            expect(result[2], equal_to(char16_t{0xDE00})); // Low surrogate
        }

        // Test conversion to char (direct copy)
        {
            auto result = from_utf8<char>("Hello");
            expect(result, equal_to("Hello"));
        }

        // Test conversion from u8string to char
        {
            const std::u8string source = u8"Test";
            auto result = from_utf8<char>(source);
            expect(result, equal_to("Test"));
        }

        // Test empty input
        {
            auto result = from_utf8<char32_t>("");
            expect(result.empty(), equal_to(true));
        }

        // Test empty std::string
        {
            const std::string empty_str;
            auto result = from_utf8<char32_t>(empty_str);
            expect(result.empty(), equal_to(true));
        }

        // Test empty output (calculated codepoints == 0)
        // This happens with invalid UTF-8 at the very start
        {
            const std::string invalid = "\xFF";
            auto result = from_utf8<char32_t>(invalid);
            expect(result.empty(), equal_to(true));
        }
    });
});

} // namespace
