// SlimLog
#include "slimlog/util/string.h"
#include "slimlog/util/unicode.h"

#include <mettle.hpp>

#include <string_view>
#include <utility>

// Test helpers
#include "helpers/common.h"

// IWYU pragma: no_include <string>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

// Constexpr test helpers
constexpr auto test_constexpr_constructors() -> bool
{
    // Default constructor
    constexpr CachedStringView<char> View1;
    static_assert(View1.data() == nullptr);
    static_assert(View1.empty());

    // String view constructor
    constexpr const char* Str = "Hello";
    constexpr std::basic_string_view<char> Sv(Str);
    constexpr CachedStringView<char> View2(Sv);
    static_assert(View2.data() == Str);
    static_assert(View2.size() == 5);

    // Move constructor
    constexpr CachedStringView<char> View3{CachedStringView<char>(Sv)};
    static_assert(View3.data() == Str);
    static_assert(View3.size() == 5);

    return true;
}

const suite<SLIMLOG_CHAR_TYPES> StringViews("string_views", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string_view<Char>;

    // Compile-time verification
    static_assert(test_constexpr_constructors());

    // Test constructors
    _.test("constructors", []() {
        // Default constructor
        const CachedStringView<Char> view1;
        expect(view1.data(), equal_to(nullptr));
        expect(view1.size(), equal_to(0U));

        // String constructor
        const auto str = from_utf8<Char>("Hello");
        const CachedStringView<Char> view2(str);
        expect(view2.data(), equal_to(str.data()));
        expect(view2.size(), equal_to(str.size()));

        // String view constructor
        const std::basic_string_view<Char> sv(str);
        const CachedStringView<Char> view3(sv);
        expect(view3.data(), equal_to(sv.data()));
        expect(view3.size(), equal_to(sv.size()));

        // Copy constructor
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const CachedStringView<Char> view4(view3);
        expect(view4.data(), equal_to(view3.data()));
        expect(view4.size(), equal_to(view3.size()));

        // Move constructor
        const CachedStringView<Char> view5(std::move(CachedStringView<Char>(str)));
        expect(view5.data(), equal_to(str.data()));
        expect(view5.size(), equal_to(str.size()));
    });

    // Test assignment operators
    _.test("assignment", []() {
        const auto str1 = from_utf8<Char>("Hello");
        const auto str2 = from_utf8<Char>("World");

        // Copy assignment
        CachedStringView<Char> view1(str1);
        CachedStringView<Char> view2;
        view2 = view1;
        expect(view2.data(), equal_to(view1.data()));
        expect(view2.size(), equal_to(view1.size()));

        // Move assignment
        CachedStringView<Char> view3;
        view3 = CachedStringView<Char>(str2);
        expect(view3.data(), equal_to(str2.data()));
        expect(view3.size(), equal_to(str2.size()));

        // String view assignment
        CachedStringView<Char> view4;
        view4 = std::basic_string_view<Char>(str1);
        expect(view4.data(), equal_to(str1.data()));
        expect(view4.size(), equal_to(str1.size()));

        // Test self assignment
        const auto old_data = view1.data();
        const auto old_size = view1.size();
        const auto old_codepoints = view1.codepoints();

        view1 = view1; // NOLINT(misc-redundant-expression)
        expect(view1.data(), equal_to(old_data));
        expect(view1.size(), equal_to(old_size));
        expect(view1.codepoints(), equal_to(old_codepoints));

        // Test self move-assignment
        view1 = std::move(view1);
        expect(view1.data(), equal_to(old_data));
        expect(view1.size(), equal_to(old_size));
        expect(view1.codepoints(), equal_to(old_codepoints));
    });

    // Test codepoint counting and caching
    _.test("codepoints", []() {
        // ASCII string
        const auto ascii = from_utf8<Char>("Hello");
        const CachedStringView<Char> ascii_view(ascii);
        expect(ascii_view.codepoints(), equal_to(5U));

        // Unicode string (Cyrillic)
        const auto cyrillic = from_utf8<Char>("Привет");
        const CachedStringView<Char> cyrillic_view(cyrillic);
        expect(cyrillic_view.codepoints(), equal_to(6U));

        // Emoji
        const auto emoji = from_utf8<Char>("Hello 😀 World");
        const CachedStringView<Char> emoji_view(emoji);
        expect(emoji_view.codepoints(), equal_to(13U));

        // Empty string
        const CachedStringView<Char> empty_view;
        expect(empty_view.codepoints(), equal_to(0U));

        // Test caching
        CachedStringView<Char> cyrillic_view2 = cyrillic_view;
        expect(cyrillic_view.codepoints(), equal_to(cyrillic_view2.codepoints()));

        // Modify underlying data by assignment and verify cache reset
        const auto cyrillic2 = from_utf8<Char>("Мир");
        cyrillic_view2 = std::basic_string_view<Char>(cyrillic2);
        expect(cyrillic_view2.codepoints(), equal_to(3U));
    });

    // Test mixed Unicode content
    _.test("mixed_unicode", []() {
        // Mix of ASCII, Cyrillic, Chinese and Emoji
        const auto mixed = from_utf8<Char>("Hello привет 你好 😀");
        const CachedStringView<Char> view(mixed);
        expect(view.codepoints(), equal_to(17U));
    });
});

const suite<SLIMLOG_CHAR_TYPES> CachedStrings("strings", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;

    // Test CachedString constructors
    _.test("constructors", []() {
        // Default constructor
        const CachedString<Char> str1;
        expect(str1.empty(), equal_to(true));
        expect(str1.size(), equal_to(0U));
        expect(str1.codepoints(), equal_to(0U));

        auto str_data = from_utf8<Char>("Hello 😀 World");
        auto str2_data = str_data;
        const auto str_codepoints
            = Util::Unicode::count_codepoints(str_data.data(), str_data.size());

        // Constructor from std::basic_string
        const CachedString<Char> str2(str_data);
        expect(std::basic_string_view<Char>(str2), equal_to(str_data));
        expect(str2.codepoints(), equal_to(str_codepoints));

        // Copy constructor - preserves cached codepoints
        CachedString<Char> str3(str2);
        expect(std::basic_string_view<Char>(str3), equal_to(str_data));
        expect(str3.codepoints(), equal_to(str_codepoints));

        // Copy constructor - with allocator
        CachedString<Char> str4(str3, std::allocator<Char>());
        expect(std::basic_string_view<Char>(str4), equal_to(str_data));
        expect(str4.codepoints(), equal_to(str_codepoints));

        // Move constructor - preserves cached codepoints
        const CachedString<Char> str5(std::move(str3));
        expect(std::basic_string_view<Char>(str5), equal_to(str_data));
        expect(str5.codepoints(), equal_to(str_codepoints));

        // Move constructor - with allocator
        const CachedString<Char> str6(std::move(str4), std::allocator<Char>());
        expect(std::basic_string_view<Char>(str6), equal_to(str_data));
        expect(str6.codepoints(), equal_to(str_codepoints));

        // Move constructor from std::basic_string
        const CachedString<Char> str7(std::move(str2_data), std::allocator<Char>());
        expect(std::basic_string_view<Char>(str7), equal_to(str_data));
        expect(str7.codepoints(), equal_to(str_codepoints));
    });

    // Test CachedString assignment operators
    _.test("assignment", []() {
        auto str_data = from_utf8<Char>("Hello 😀 World");
        auto str2_data = str_data;
        const auto str_codepoints
            = Util::Unicode::count_codepoints(str_data.data(), str_data.size());

        // Copy assignment - preserves cached codepoints
        CachedString<Char> str1(str_data);
        CachedString<Char> str2;
        str2 = str1;
        expect(std::basic_string_view<Char>(str2), equal_to(str_data));
        expect(str2.codepoints(), equal_to(str_codepoints));

        // Move assignment - preserves cached codepoints
        CachedString<Char> str3(str_data);
        CachedString<Char> str4;
        str4 = std::move(str3);
        expect(std::basic_string_view<Char>(str4), equal_to(str_data));
        expect(str4.codepoints(), equal_to(str_codepoints));

        // Assignment from std::basic_string  - invalidates cache
        CachedString<Char> str5;
        str5 = str_data;
        expect(std::basic_string_view<Char>(str5), equal_to(str_data));
        expect(str5.codepoints(), equal_to(str_codepoints));

        // Move assignment from std::basic_string - invalidates cache
        str5 = std::move(str2_data);
        expect(std::basic_string_view<Char>(str5), equal_to(str_data));
        expect(str5.codepoints(), equal_to(str_codepoints));

        // Self assignment
        str1 = str1; // NOLINT(misc-redundant-expression)
        expect(std::basic_string_view<Char>(str1), equal_to(str_data));
        expect(str1.codepoints(), equal_to(str_codepoints));

        // Self move-assignment
        str1 = std::move(str1); // NOLINT(misc-redundant-expression)
        expect(std::basic_string_view<Char>(str1), equal_to(str_data));
        expect(str1.codepoints(), equal_to(str_codepoints));
    });

    // Test CachedString codepoint counting and caching
    _.test("codepoints", []() {
        // ASCII string
        const auto ascii = from_utf8<Char>("Hello");
        const CachedString<Char> ascii_str(ascii);
        expect(ascii_str.codepoints(), equal_to(5U));
        // Second call should use cached value
        expect(ascii_str.codepoints(), equal_to(5U));

        // Unicode string (Cyrillic)
        const auto cyrillic = from_utf8<Char>("Привет");
        const CachedString<Char> cyrillic_str(cyrillic);
        expect(cyrillic_str.codepoints(), equal_to(6U));

        // Emoji
        const auto emoji = from_utf8<Char>("Hello 😀 World");
        const CachedString<Char> emoji_str(emoji);
        expect(emoji_str.codepoints(), equal_to(13U));

        // Empty string
        const CachedString<Char> empty_str;
        expect(empty_str.codepoints(), equal_to(0U));

        // Test cache preservation after copy
        CachedString<Char> cyrillic_str2 = cyrillic_str;
        expect(cyrillic_str2.codepoints(), equal_to(6U));

        // Reset cache by assigning empty string
        cyrillic_str2 = CachedString<Char>();
        expect(cyrillic_str2.codepoints(), equal_to(0U));

        // Test cache preservation after move
        CachedString<Char> cyrillic_str3(cyrillic);
        std::ignore = cyrillic_str3.codepoints(); // Calculate cache
        const CachedString<Char> cyrillic_str4(std::move(cyrillic_str3));
        expect(cyrillic_str4.codepoints(), equal_to(6U));
    });

    // Test CachedString modification invalidates cache
    _.test("cache_invalidation", []() {
        const auto initial = from_utf8<Char>("Hello");
        CachedString<Char> str(initial);

        // Calculate and cache codepoints
        expect(str.codepoints(), equal_to(5U));

        // Assignment from std::basic_string invalidates cache
        const std::basic_string<Char> std_str(from_utf8<Char>("Привет"));
        str = std_str;
        expect(str.codepoints(), equal_to(6U));

        // Move assignment from std::basic_string also invalidates cache
        std::basic_string<Char> std_str2(from_utf8<Char>("你好"));
        str = std::move(std_str2);
        expect(str.codepoints(), equal_to(2U));
    });

    // Test CachedString with mixed Unicode
    _.test("mixed_unicode", []() {
        // Mix of ASCII, Cyrillic, Chinese and Emoji
        const auto mixed = from_utf8<Char>("Hello привет 你好 😀");
        const CachedString<Char> str(mixed);
        expect(str.codepoints(), equal_to(17U));

        // Copy should preserve cache
        CachedString<Char> str2 = str;
        expect(str2.codepoints(), equal_to(17U));

        // Reset to empty
        str2 = CachedString<Char>();
        expect(str2.codepoints(), equal_to(0U));
        expect(str2.empty(), equal_to(true));
    });

    // Test explicit conversion to CachedStringView and std::basic_string_view
    _.test("to_string_view", []() {
        const auto str1_data = from_utf8<Char>("Test String");
        const auto str1_codepoints
            = Util::Unicode::count_codepoints(str1_data.data(), str1_data.size());

        const auto str2_data = from_utf8<Char>("New String2");
        const auto str2_codepoints
            = Util::Unicode::count_codepoints(str2_data.data(), str2_data.size());

        CachedString<Char> str1(str1_data);
        expect(std::basic_string_view<Char>(str1), equal_to(str1_data));
        expect(str1.codepoints(), equal_to(str1_codepoints));

        // Convert to CachedStringView
        CachedStringView<Char> view;
        view = CachedStringView<Char>(str1);
        expect(std::basic_string_view<Char>(view), equal_to(str1_data));
        expect(view.codepoints(), equal_to(str1_codepoints));

        // Check that is still linked to original CachedString
        str1 = str2_data;
        expect(std::basic_string_view<Char>(view), equal_to(str2_data));
        expect(view.codepoints(), equal_to(str2_codepoints));

        // Convert from basic_string_view explicitly
        view = std::basic_string_view<Char>(str1_data);
        expect(view.codepoints(), equal_to(str1_codepoints));
        expect(std::basic_string_view<Char>(view), equal_to(str1_data));

        // Convert from basic_string implicitly
        view = str2_data;
        expect(view.codepoints(), equal_to(str2_codepoints));
        expect(std::basic_string_view<Char>(view), equal_to(str2_data));
    });
});

} // namespace
