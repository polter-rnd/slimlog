// SlimLog
#include "slimlog/pattern.h"

#include "slimlog/format.h"
#include "slimlog/record.h"
#include "slimlog/sink.h"

// Test helpers
#include "helpers/common.h"

#include <mettle.hpp>

#include <atomic>
#include <chrono>
#include <initializer_list>
#include <latch>
#include <random>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// IWYU pragma: no_include <functional>
// IWYU pragma: no_include <utility>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

// Helper to create test record
template<typename Char>
auto create_test_record(
    Level level = Level::Info,
    std::basic_string_view<Char> message = from_utf8<Char>("Test message"),
    std::basic_string_view<Char> category = from_utf8<Char>("test_category"),
    std::size_t thread_id = 12345,
    std::pair<std::chrono::sys_seconds, std::uint64_t> time = time_mock(),
    std::source_location location = std::source_location::current()) -> Record<Char>
{
    static thread_local std::basic_string<Char> message_data;
    static thread_local std::basic_string<Char> category_data;

    message_data = message;
    category_data = category;

    Record<Char> record;
    record.level = level;
    record.filename = RecordStringView<char>{location.file_name()};
    record.function = RecordStringView<char>{location.function_name()};
    record.line = location.line();
    record.category = RecordStringView<Char>{category_data.data(), category_data.size()};
    record.thread_id = thread_id;
    record.time = time;
    record.message = RecordStringView<Char>{message_data.data(), message_data.size()};
    return record;
}

const suite<SLIMLOG_CHAR_TYPES> PatternTests("pattern", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string_view<Char>;
    using StringView = std::basic_string_view<Char>;
    using PatternType = Pattern<Char>;
    using BufferType = FormatBuffer<Char, DefaultBufferSize>;

    // Test empty pattern
    _.test("empty_pattern", []() {
        PatternType pattern;
        expect(pattern.empty(), equal_to(true));

        BufferType buffer;
        auto record = create_test_record<Char>();
        pattern.format(buffer, record);

        // Empty pattern should default to message only
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("Test message")));
    });

    // Test empty string handling
    _.test("empty_strings", []() {
        const auto pattern_str = from_utf8<Char>("{message}{file}{function}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>(Level::Info, StringView{});
        record.filename = RecordStringView<char>{""}; // Empty filename
        record.function = RecordStringView<char>{""}; // Empty function
        pattern.format(buffer, record);

        // Empty message should result in empty output
        expect(StringView(buffer.data(), buffer.size()), equal_to(StringView{}));
        expect(buffer.size(), equal_to(0));
    });

    // Test basic placeholders
    _.test("basic_placeholders", []() {
        const auto pattern_str = from_utf8<Char>("[{level}] {category}: {message}");
        PatternType pattern(pattern_str);
        expect(pattern.empty(), equal_to(false));

        BufferType buffer;
        auto record = create_test_record<Char>();
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
        auto record = create_test_record<Char>();
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
        fields.message = record.message;

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

        PatternType pattern(from_utf8<Char>("[{level}] {message:s}"), custom_levels);

        BufferType buffer;
        auto record = create_test_record<Char>();

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
        const auto pattern_str = from_utf8<Char>("[{level:^10s}] [{category:<15}] {message:*>16}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>();
        pattern.format(buffer, record);

        const auto result = StringView(buffer.data(), buffer.size());
        expect(
            result, equal_to(from_utf8<Char>("[   INFO   ] [test_category  ] ****Test message")));
    });

    // Test no padding when field width is less than content length
    _.test("alignment_overflow", []() {
        const auto pattern_str = from_utf8<Char>("{message:5}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record
            = create_test_record<Char>(Level::Info, from_utf8<Char>("Hello!")); // 6 codepoints

        pattern.format(buffer, record);

        // Since spec_width (5) < codepoints (6), no padding should be added
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("Hello!")));
    });

    // Test field alignment and padding
    _.test("alignment_unicode", []() {
        const auto pattern_str
            = from_utf8<Char>(u8"[{level:😀^8}] [{category:<15}] {message:∮>15}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>(Level::Info, from_utf8<Char>(u8"𝒽𝑒𝓁𝓁𝑜 🌍🚀💫!"));
        pattern.format(buffer, record);

        const auto result = StringView(buffer.data(), buffer.size());
        expect(
            result,
            equal_to(from_utf8<Char>(u8"[😀😀INFO😀😀] [test_category  ] ∮∮∮∮∮𝒽𝑒𝓁𝓁𝑜 🌍🚀💫!")));
    });

    // Test escaped braces
    _.test("escaped_braces", []() {
        const auto pattern_str = from_utf8<Char>("{{level}} {level} {{message}}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>();
        pattern.format(buffer, record);

        const auto expected = from_utf8<Char>("{level} INFO {message}");
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test time formatting
    _.test("time_formatting", []() {
        const auto pattern_str = from_utf8<Char>("{time:%Y-%m-%d %H:%M:%S} {message}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>();
        pattern.format(buffer, record);

        PatternFields<Char> fields;
        fields.time = record.time.first;
        fields.nsec = record.time.second;
        fields.message = record.message;

        const auto expected = pattern_format<Char>(pattern_str, fields);
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test time components
    _.test("time_components", []() {
        const auto pattern_str = from_utf8<Char>("{msec}ms {usec}us {nsec}ns");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>();
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
        auto record = create_test_record<Char>();
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
        auto record = create_test_record<Char>();

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
            auto record = create_test_record<Char>();
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

        // Unexpected character after string specifier
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{level:10s_}")); },
            thrown<FormatError>());

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

        // Invalid fill character '{'
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{level:{<10}")); },
            thrown<FormatError>());

        // Invalid invalid Unicode fill character (claims 4 bytes but only has 2)
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{level:\xF0\x9F}")); },
            thrown<FormatError>());
    });

    // Test integer overflow in field width parsing
    _.test("large_field_width", []() {
        // Test with a number larger than std::numeric_limits<int>::max()
        // std::numeric_limits<int>::max() is typically 2147483647
        // Using 99999999999999999999 which is much larger
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{level:99999999999999999999}")); },
            thrown<FormatError>());

        // Test with maximum int + 1 (2147483648 for 32-bit int)
        expect(
            []() { const PatternType pattern(from_utf8<Char>("{category:<2147483648}")); },
            thrown<FormatError>());

        // Test that maximum valid width still works by actually formatting
        PatternType pattern(from_utf8<Char>("{level:2147483647}"));
        BufferType buffer;
        auto record = create_test_record<Char>();
        pattern.format(buffer, record);

        // Should format successfully with maximum int width
        // The result will be "INFO" followed by many spaces (2147483643 spaces)
        expect(
            StringView(buffer.data(), buffer.size()).substr(0, 4),
            equal_to(from_utf8<Char>("INFO")));
        expect(buffer.size(), equal_to(2147483647));
    });

    // Test complex pattern
    _.test("complex_pattern", []() {
        const auto pattern_str
            = from_utf8<Char>("[{time:%Y-%m-%d %H:%M:%S}.{msec:03}] [{level:>5}] {category} "
                              "({file:^100}:{line}) {function:>100}: {message}");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>();
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
        fields.message = record.message;

        const auto expected = pattern_format<Char>(pattern_str, fields);
        expect(StringView(buffer.data(), buffer.size()), equal_to(expected));
    });

    // Test pattern with only text (no placeholders)
    _.test("text_only_pattern", []() {
        const auto pattern_str = from_utf8<Char>("This is a static log message");
        PatternType pattern(pattern_str);

        BufferType buffer;
        auto record = create_test_record<Char>();
        pattern.format(buffer, record);

        expect(StringView(buffer.data(), buffer.size()), equal_to(pattern_str));
    });

    // Test concurrent formatting, ensure CachedFormatter thread-safety
    _.test("pattern_thread_safety", []() {
        constexpr int NumThreads = 8;
        constexpr int IterationsPerThread = 500;

        // Create a pattern with fields that are processed by CachedFormatter internally:
        // thread_id, time components (msec, usec, nsec) - these use CachedFormatter for formatting
        const auto pattern_str = from_utf8<Char>("Thread {thread}: {level}|{time:%Y-%m-%d "
                                                 "%X}.{msec:_>#10x}:{nsec:_^#16o}:{usec:_<#24b}");
        PatternType pattern(pattern_str);

        std::vector<std::thread> threads;
        std::latch start_latch(NumThreads + 1);
        std::atomic<int> successful_formats{0};

        threads.reserve(NumThreads);
        for (int i = 0; i < NumThreads; ++i) {
            threads.emplace_back([&, thread_id = i]() {
                start_latch.arrive_and_wait();

                BufferType local_buffer;
                std::random_device rd;
                std::mt19937 gen(rd() + thread_id);
                std::uniform_int_distribution<std::uint64_t> nsec_dist(0, 999999999);

                for (int j = 0; j < IterationsPerThread; ++j) {
                    local_buffer.clear();

                    // Create records with varying thread_ids and time components to trigger
                    // CachedFormatter buffer switching and spinlock contention
                    auto base_time = std::chrono::sys_seconds(
                        std::chrono::seconds(1609459200 + j)); // 2021-01-01 + j seconds
                    auto nsec_value = nsec_dist(gen); // Random nanoseconds
                    auto record = create_test_record<Char>(
                        Level::Info,
                        {},
                        {},
                        1000 + (thread_id * 100) + (j % 50), // Random thread ID
                        std::make_pair(base_time, nsec_value));
                    PatternFields<Char> fields;
                    fields.level = from_utf8<Char>("INFO");
                    fields.thread_id = record.thread_id;
                    fields.time = record.time.first;
                    fields.nsec = record.time.second;

                    // This will internally use CachedFormatter for thread_id, msec, usec, nsec
                    // formatting Multiple threads formatting different values will cause
                    // spinlock contention
                    pattern.format(local_buffer, record);

                    const auto actual = StringView{local_buffer.data(), local_buffer.size()};
                    const auto expected = pattern_format<Char>(pattern_str, fields);
                    expect(actual, equal_to(expected));

                    successful_formats.fetch_add(1, std::memory_order_relaxed);

                    // Add occasional yields to increase lock contention
                    if (j % 25 == 0) {
                        std::this_thread::yield();
                    }
                }
            });
        }

        // Start all threads simultaneously to maximize contention
        start_latch.arrive_and_wait();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Verify all formatting operations completed successfully
        expect(successful_formats.load(), equal_to(NumThreads * IterationsPerThread));
    });
});

} // namespace
