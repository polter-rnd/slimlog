// SlimLog
#include "slimlog/pattern.h"

#include "slimlog/record.h"
#include "slimlog/sink.h"
#include "slimlog/util/buffer.h"

// Test helpers
#include "helpers/common.h"

#include <mettle.hpp>

#include <array>
#include <string>
#include <string_view>

// IWYU pragma: no_include <utility>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

// Helper to create test record
template<typename String, typename Char>
auto create_test_record() -> Record<String, Char>
{
    using RecordType = Record<String, Char>;
    using StringViewType = std::basic_string_view<Char>;

    static const auto* const filename_str = "test.cpp";
    static const auto* const function_str = "test_function";
    static const auto category_str = from_utf8<Char>("test_category");
    static const auto message_str = from_utf8<Char>("Test message");

    RecordType record;
    record.level = Level::Info;
    record.filename = RecordStringView<char>{filename_str};
    record.function = RecordStringView<char>{function_str};
    record.line = 42;
    record.category = RecordStringView<Char>{category_str};
    record.thread_id = 12345;
    record.time = time_mock();
    record.message = RecordStringView<Char>{message_str};
    return record;
}

const suite<SLIMLOG_CHAR_TYPES> PatternTests("pattern", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;
    using StringView = std::basic_string_view<Char>;
    using PatternType = Pattern<Char>;
    using BufferType = typename Util::MemoryBuffer<Char, DefaultBufferSize>;

    // Test empty pattern
    _.test("empty_pattern", []() {
        PatternType pattern;
        expect(pattern.empty(), equal_to(true));

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        // Empty pattern should default to message only
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("Test message")));
    });

    // Test basic placeholders
    _.test("basic_placeholders", []() {
        const auto pattern_str = from_utf8<Char>("[{level}] {category}: {message}");
        PatternType pattern(pattern_str);
        expect(pattern.empty(), equal_to(false));

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        const auto expected = from_utf8<Char>("[INFO] test_category: Test message");
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test all placeholder types
    _.test("all_placeholders", []() {
        const auto pattern_str = from_utf8<Char>("{category}|{level}|{file}|{line}|{function}|{"
                                                 "thread}|{time}|{msec}|{usec}|{nsec}|{message}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        PatternFields<Char> fields;
        fields.category = record.category;
        fields.level = from_utf8<Char>("INFO");
        fields.file
            = from_utf8<Char>(std::string_view(record.filename.data(), record.filename.size()));
        fields.line = record.line;
        fields.function
            = from_utf8<Char>(std::string_view(record.function.data(), record.function.size()));
        fields.thread_id = record.thread_id;
        fields.time = record.time.first;
        fields.nsec = record.time.second;
        fields.message = std::get<RecordStringView<Char>>(record.message);

        const auto expected = pattern_format<Char>(pattern_str, fields);
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test custom level names
    _.test("custom_level_names", []() {
        const std::vector<std::pair<Level, std::basic_string<Char>>> custom_levels
            = {{Level::Trace, from_utf8<Char>("CUSTOM_TRACE")},
               {Level::Debug, from_utf8<Char>("CUSTOM_DEBUG")},
               {Level::Info, from_utf8<Char>("CUSTOM_INFO")},
               {Level::Warning, from_utf8<Char>("CUSTOM_WARN")},
               {Level::Error, from_utf8<Char>("CUSTOM_ERROR")},
               {Level::Fatal, from_utf8<Char>("CUSTOM_FATAL")}};

        PatternType pattern(from_utf8<Char>("[{level}] {message}"), custom_levels);

        BufferType buffer;
        auto record = create_test_record<String, Char>();

        for (const auto& [level, level_name] : custom_levels) {
            buffer.clear();
            record.level = level;
            pattern.format(buffer, record);
            const auto expected
                = from_utf8<Char>("[") + level_name + from_utf8<Char>("] Test message");
            expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
        }
    });

    // Test field alignment and padding
    _.test("alignment", []() {
        const auto pattern_str = from_utf8<Char>("[{level:^10}] [{category:<15}] {message:*>16}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        const auto result = StringView(buffer.data(), buffer.size());
        expect(
            result, equal_to(from_utf8<Char>("[   INFO   ] [test_category  ] ****Test message")));
    });

    // Test escaped braces
    _.test("escaped_braces", []() {
        const auto pattern_str = from_utf8<Char>("{{level}} {level} {{message}}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        const auto expected = from_utf8<Char>("{level} INFO {message}");
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test time formatting
    _.test("time_formatting", []() {
        const auto pattern_str = from_utf8<Char>("{time:%Y-%m-%d %H:%M:%S} {message}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        PatternFields<Char> fields;
        fields.time = record.time.first;
        fields.nsec = record.time.second;
        fields.message = std::get<RecordStringView<Char>>(record.message);

        const auto expected = pattern_format<Char>(pattern_str, fields);
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test time components
    _.test("time_components", []() {
        const auto pattern_str = from_utf8<Char>("{msec}ms {usec}us {nsec}ns");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        PatternFields<Char> fields;
        fields.time = record.time.first;
        fields.nsec = record.time.second;

        const auto expected = pattern_format<Char>(pattern_str, fields);
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test set_pattern method
    _.test("set_pattern", []() {
        PatternType pattern;
        expect(pattern.empty(), equal_to(true));

        pattern.set_pattern(from_utf8<Char>("[{level}] {message}"));
        expect(pattern.empty(), equal_to(false));

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        const auto expected = from_utf8<Char>("[INFO] Test message");
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test set_levels method
    _.test("set_levels", []() {
        PatternType pattern(from_utf8<Char>("{level}"));

        pattern.set_levels(
            std::make_pair(Level::Info, from_utf8<Char>("CUSTOM_INFO")),
            std::make_pair(Level::Debug, from_utf8<Char>("CUSTOM_DEBUG")));

        BufferType buffer;
        auto record = create_test_record<String, Char>();

        record.level = Level::Info;
        pattern.format(buffer, record);
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("CUSTOM_INFO")));

        buffer.clear();
        record.level = Level::Debug;
        pattern.format(buffer, record);
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("CUSTOM_DEBUG")));
    });

    // Test all log levels
    _.test("all_log_levels", []() {
        PatternType pattern(from_utf8<Char>("{level}"));

        const auto levels
            = {Level::Trace, Level::Debug, Level::Info, Level::Warning, Level::Error, Level::Fatal};

        const auto expected_names
            = {from_utf8<Char>("TRACE"),
               from_utf8<Char>("DEBUG"),
               from_utf8<Char>("INFO"),
               from_utf8<Char>("WARN"),
               from_utf8<Char>("ERROR"),
               from_utf8<Char>("FATAL")};

        auto expected_it = expected_names.begin();
        for (auto level : levels) {
            BufferType buffer;
            auto record = create_test_record<String, Char>();
            record.level = level;
            pattern.format(buffer, record);

            expect(StringView(buffer.data(), buffer.size()), equal_to(*expected_it));
            ++expected_it;
        }
    });

    // Test format errors
    _.test("format_errors", []() {
        // Unmatched opening brace
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{level")); }, thrown<FormatError>());

        // Unmatched closing brace
        expect(
            []() { const PatternType pattern(from_utf8<Char>("level}")); }, thrown<FormatError>());

        // Unknown placeholder
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{unknown}")); },
            thrown<FormatError>());

        // Invalid format spec
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{level:invalid}")); },
            thrown<FormatError>());
    });

    // Test complex pattern
    _.test("complex_pattern", []() {
        const auto pattern_str
            = from_utf8<Char>("[{time:%Y-%m-%d %H:%M:%S}.{msec:03}] [{level:>5}] {category} "
                              "({file}:{line}) {function}: {message}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        PatternFields<Char> fields;
        fields.category = record.category;
        fields.level = from_utf8<Char>("INFO");
        fields.file
            = from_utf8<Char>(std::string_view(record.filename.data(), record.filename.size()));
        fields.line = record.line;
        fields.function
            = from_utf8<Char>(std::string_view(record.function.data(), record.function.size()));
        fields.time = record.time.first;
        fields.nsec = record.time.second;
        fields.message = std::get<RecordStringView<Char>>(record.message);

        const auto expected = pattern_format<Char>(pattern_str, fields);
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test pattern with only text (no placeholders)
    _.test("text_only_pattern", []() {
        const auto pattern_str = from_utf8<Char>("This is a static log message");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<String, Char>();
        pattern.format(buffer, record);

        expect(StringView(buffer.data(), buffer.size()), equal_to(pattern_str));
    });

    // Test unicode in patterns
    _.test("unicode_pattern", []() {
        PatternType pattern;
        BufferType buffer;

        for (const auto& unicode_str : unicode_strings<Char>()) {
            buffer.clear();
            auto record = create_test_record<String, Char>();
            record.message = RecordStringView<Char>{unicode_str};

            pattern.set_pattern(from_utf8<Char>("{message}"));
            pattern.format(buffer, record);

            expect(StringView(buffer.data(), buffer.size()), equal_to(unicode_str));
        }
    });
});

} // namespace