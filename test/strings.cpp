// SlimLog
#include "slimlog/util/string.h"

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

const suite<SLIMLOG_CHAR_TYPES> Strings("strings", type_only, [](auto& _) {
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

} // namespace