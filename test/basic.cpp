// SlimLog
#include "slimlog/format.h"
#include "slimlog/logger.h"
#include "slimlog/sinks/callback_sink.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"
#include "slimlog/util/os.h"

// Test helpers
#include "helpers/common.h"
#include "helpers/file_capturer.h"
#include "helpers/stream_capturer.h"

#include <mettle.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

#ifdef SLIMLOG_FMTLIB
#include <fmt/format.h>
#include <fmt/xchar.h> // IWYU pragma: keep
#else
#include <format>
#endif

// IWYU pragma: no_include <utility>
// IWYU pragma: no_include <fstream>
// IWYU pragma: no_include <functional>
// IWYU pragma: no_include <memory>
// IWYU pragma: no_include <vector>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

const suite<SLIMLOG_CHAR_TYPES> Basic("basic", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;

    static auto log_filename = get_log_filename<Char>("basic");
    std::filesystem::remove(log_filename);

    // Test empty message
    _.test("empty_message", []() {
        StreamCapturer<Char> cap_out;
        auto log = Logger<String>::create();
        log->template add_sink<OStreamSink>(cap_out);

        const auto empty_message = String{};
        log->info(empty_message);

        expect(cap_out.read(), equal_to(String{Char{'\n'}}));
    });

    // Test logger categories
    _.test("categories", []() {
        StreamCapturer<Char> cap_out;
        const auto pattern = from_utf8<Char>("[{category}] {message}");

        const auto default_category = from_utf8<Char>("default");
        const auto custom_category = from_utf8<Char>("my_module");

        auto default_log = Logger<String>::create();
        auto custom_log = Logger<String>::create(custom_category);
        expect(custom_log->category(), equal_to(custom_category));

        default_log->template add_sink<OStreamSink>(cap_out, pattern);
        custom_log->template add_sink<OStreamSink>(cap_out, pattern);

        const auto message = from_utf8<Char>("Test message");

        default_log->info(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[default] ") + message + Char{'\n'}));

        custom_log->info(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[my_module] ") + message + Char{'\n'}));
    });

    // Test all constructors
    _.test("constructors", []() {
        // Default constructor
        auto log = Logger<String>::create();
        expect(log->category(), equal_to(from_utf8<Char>("default")));
        expect(log->level(), equal_to(Level::Info));

        // Constructor with custom category and level
        const auto custom_category = from_utf8<Char>("test_category");
        log = Logger<String>::create(custom_category, Level::Debug);
        expect(log->category(), equal_to(custom_category));
        expect(log->level(), equal_to(Level::Debug));

        // Constructor with category only
        log = Logger<String>::create(custom_category);
        expect(log->category(), equal_to(custom_category));
        expect(log->level(), equal_to(Level::Info));

        // Constructor with level only
        log = Logger<String>::create(Level::Warning);
        expect(log->category(), equal_to(from_utf8<Char>("default")));
        expect(log->level(), equal_to(Level::Warning));

        // Constructor with parent logger, custom category and level
        const auto child_category = from_utf8<Char>("log_child");
        auto log_child = Logger<String>::create(log, child_category, Level::Error);
        expect(log_child->category(), equal_to(child_category));
        expect(log_child->level(), equal_to(Level::Error));

        // Constructor with parent logger, level inherited from parent
        log_child = Logger<String>::create(log, child_category);
        expect(log_child->category(), equal_to(child_category));
        expect(log_child->level(), equal_to(log->level()));

        // Constructor with parent logger, category inherited from parent
        log_child = Logger<String>::create(log, Level::Error);
        expect(log_child->category(), equal_to(log->category()));
        expect(log_child->level(), equal_to(Level::Error));

        // Constructor with parent logger, level and category inherited from parent
        log_child = Logger<String>::create(log);
        expect(log_child->category(), equal_to(log->category()));
        expect(log_child->level(), equal_to(log->level()));
    });

    // Test convenience logging methods
    _.test("convenience_methods", []() {
        StreamCapturer<Char> cap_out;
        const auto pattern = from_utf8<Char>("[{level}] {message}");

        auto log = Logger<String>::create(Level::Trace);
        log->template add_sink<OStreamSink>(cap_out, pattern);

        const auto message = from_utf8<Char>("Test message");

        log->trace(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[TRACE] ") + message + Char{'\n'}));

        log->debug(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[DEBUG] ") + message + Char{'\n'}));

        log->info(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[INFO] ") + message + Char{'\n'}));

        log->warning(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[WARN] ") + message + Char{'\n'}));

        log->error(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[ERROR] ") + message + Char{'\n'}));

        log->fatal(message);
        expect(cap_out.read(), equal_to(from_utf8<Char>("[FATAL] ") + message + Char{'\n'}));
    });

    // Test multiple sinks
    _.test("multiple_sinks", []() {
        StreamCapturer<Char> cap_out1;
        StreamCapturer<Char> cap_out2;

        auto log = Logger<String>::create();
        auto sink1 = log->template add_sink<OStreamSink>(cap_out1);
        auto sink2 = log->template add_sink<OStreamSink>(cap_out2);

        const auto message = from_utf8<Char>("Multi-sink message");

        log->info(message);

        // Both sinks should receive the same message
        expect(cap_out1.read(), equal_to(message + Char{'\n'}));
        expect(cap_out2.read(), equal_to(message + Char{'\n'}));

        // Remove one sink
        expect(log->remove_sink(sink1), equal_to(true));

        log->info(message);

        // Only second sink should receive the message
        expect(cap_out1.read(), equal_to(String{}));
        expect(cap_out2.read(), equal_to(message + Char{'\n'}));

        // Remove last sink
        expect(log->remove_sink(sink2), equal_to(true));

        log->info(message);

        // No sinks should receive the message now
        expect(cap_out1.read(), equal_to(String{}));
        expect(cap_out2.read(), equal_to(String{}));
    });

    // Test sink management
    _.test("sink_management", []() {
        auto log = Logger<String>::create();
        StreamCapturer<Char> cap_out;

        // Add sink
        auto sink = log->template add_sink<OStreamSink>(cap_out);
        expect(sink, is_not(nullptr));

        // Test sink enabled/disabled functionality
        const auto message = from_utf8<Char>("Test message");

        // Sink should be enabled by default
        expect(log->sink_enabled(sink), equal_to(true));

        // Test logging when sink is enabled
        log->info(message);
        expect(cap_out.read(), equal_to(message + Char{'\n'}));

        // Disable the sink
        expect(log->set_sink_enabled(sink, false), equal_to(true));
        expect(log->sink_enabled(sink), equal_to(false));

        // Test logging when sink is disabled - should not produce output
        log->info(message);
        expect(cap_out.read(), equal_to(String{}));

        // Re-enable the sink
        expect(log->set_sink_enabled(sink, true), equal_to(true));
        expect(log->sink_enabled(sink), equal_to(true));

        // Test logging when sink is re-enabled
        log->info(message);
        expect(cap_out.read(), equal_to(message + Char{'\n'}));

        // Test set_sink_enabled with non-existent sink
        auto non_existent_sink = std::make_shared<OStreamSink<String>>(cap_out);
        expect(log->set_sink_enabled(non_existent_sink, false), equal_to(false));

        // Test sink_enabled with non-existent sink
        expect(log->sink_enabled(non_existent_sink), equal_to(false));

        // Remove existing sink
        expect(log->remove_sink(sink), equal_to(true));

        // Try to remove the same sink again
        expect(log->remove_sink(sink), equal_to(false));

        // Try to remove null sink
        expect(log->remove_sink(nullptr), equal_to(false));
    });

    _.test("levels", []() {
        StreamCapturer<Char> cap_out;
        auto log = Logger<String>::create();
        log->template add_sink<OStreamSink>(cap_out);
        const auto levels = {
            Level::Trace,
            Level::Debug,
            Level::Info,
            Level::Warning,
            Level::Error,
            Level::Fatal,
        };

        std::for_each(levels.begin(), levels.end(), [&log, &levels, &cap_out](auto& log_level) {
            log->set_level(log_level);
            expect(log->level(), equal_to(log_level));

            const auto message = from_utf8<Char>("Hello, World!");
            for (const auto msg_level : levels) {
                log->message(msg_level, message);
                expect(log->level_enabled(msg_level), equal_to(msg_level <= log_level));
                if (msg_level > log_level) {
                    expect(cap_out.read(), equal_to(String{}));
                } else {
                    expect(cap_out.read(), equal_to(message + Char{'\n'}));
                }
            }
        });
    });

    _.test("null_sink", []() {
        auto log = Logger<String>::create();
        auto null_sink = log->template add_sink<NullSink>();
        log->info(from_utf8<Char>("Hello, World!"));
        null_sink->flush();
        expect(log->remove_sink(null_sink), equal_to(true));
    });

    _.test("ostream_sink", []() {
        StreamCapturer<Char> cap_out;
        auto log = Logger<String>::create();

        auto ostream_sink = log->template add_sink<OStreamSink>(cap_out);
        for (const auto& message : unicode_strings<Char>()) {
            log->info(message);
            ostream_sink->flush();
            expect(cap_out.read(), equal_to(message + Char{'\n'}));
        }
    });

    _.test("file_sink", []() {
        auto log = Logger<String>::create();
        // Check that invalid path throws an error
        expect([&log]() { log->template add_sink<FileSink>(""); }, thrown<std::system_error>());
        // Check that valid path works
        FileCapturer<Char> cap_file(log_filename);

        auto file_sink = log->template add_sink<FileSink>(cap_file.path().string());
        for (const auto& message : unicode_strings<Char>()) {
            log->info(message);
            file_sink->flush();
            expect(cap_file.read(), equal_to(message + Char{'\n'}));
        }
    });

    _.test("callback_sink", []() {
        auto log = Logger<String>::create();
        Level captured_level{Level::Debug};
        Location captured_location;
        String captured_message;

        // Create callback that captures the output
        auto callback = [&](Level level, Location location, std::basic_string_view<Char> message) {
            captured_level = level;
            captured_location = location;
            captured_message = message;
        };

        auto callback_sink = log->template add_sink<CallbackSink>(callback);

        for (const auto& message : unicode_strings<Char>()) {
            captured_message.clear();
            log->info(message);
            expect(captured_location.line(), equal_to(__builtin_LINE() - 1));
            expect(captured_location.file_name(), equal_to(std::string_view{"basic.cpp"}));
            expect(
                captured_location.function_name(),
                equal_to(std::string_view{__builtin_FUNCTION()}));
            expect(captured_message, equal_to(message + Char{'\n'}));
            expect(captured_level, equal_to(Level::Info));
        }
    });

    // Basic pattern test
    _.test("pattern", []() {
        FileCapturer<Char> cap_file(log_filename);
        const auto pattern = from_utf8<Char>("({category}) [{level:>10}] "
                                             "<{time:%Y/%d/%m %T} {msec}ms={usec}us={nsec}ns> "
                                             "#{thread} {function} {file}|{line}: {message}");

        auto log = Logger<String>::create();
        log->set_time_func(time_mock);
        auto file_sink = std::static_pointer_cast<FormattableSink<String>>(
            log->template add_sink<FileSink>(cap_file.path().string()));
        file_sink->set_pattern(pattern);

        PatternFields<Char> fields;
        fields.category = from_utf8<Char>("default");
        fields.level = from_utf8<Char>("INFO");
        fields.thread_id = Util::OS::thread_id();
        fields.function = from_utf8<Char>(__builtin_FUNCTION());
        fields.file = from_utf8<Char>("basic.cpp");
        fields.time = time_mock().first;
        fields.nsec = time_mock().second;

        for (const auto& message : unicode_strings<Char>()) {
            log->info(message);
            fields.line = __builtin_LINE() - 1;
            fields.message = message;
            file_sink->flush();
            expect(cap_file.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });

    // Test with simple {time} format (no format specified)
    _.test("simple_time_pattern", []() {
        StreamCapturer<Char> cap_out;

        const auto pattern = from_utf8<Char>("[{level}] {time} - {message}");
        const auto message = from_utf8<Char>("Warning message");

        auto log = Logger<String>::create();
        log->set_time_func(time_mock);
        log->template add_sink<OStreamSink>(cap_out, pattern);

        PatternFields<Char> fields;
        fields.level = from_utf8<Char>("WARN");
        fields.time = time_mock().first;
        fields.nsec = time_mock().second;

        for (const auto& message : unicode_strings<Char>()) {
            log->warning(message);
            fields.message = message;
            expect(cap_out.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });

    // Test formatted message with multiple types
    _.test("format", []() {
        StreamCapturer<Char> cap_out;
        const FileCapturer<Char> cap_file(log_filename);

        const auto pattern = from_utf8<Char>("[{level}] {time} - {message}");

        auto log = Logger<String>::create();
        log->set_time_func(time_mock);
        log->template add_sink<OStreamSink>(cap_out, pattern);
        log->template add_sink<FileSink>(cap_file.path().string(), pattern);

        PatternFields<Char> fields;
        fields.level = from_utf8<Char>("INFO");
        fields.time = time_mock().first;
        fields.nsec = time_mock().second;

        static constexpr std::array<Char, 57> FmtMessage{
            'M', 'e', 's', 's', 'a', 'g', 'e', ' ', 'f', 'r', 'o', 'm', ' ', '{', '}',
            ':', ' ', '{', ':', '_', '>', '5', '0', '}', ' ', 'T', 'h', 'e', ' ', 'a',
            'n', 's', 'w', 'e', 'r', ' ', 'i', 's', ' ', '{', ':', '#', 'x', '}', ',',
            ' ', '1', '0', '/', '3', '=', '{', ':', '.', '3', '}', '\0'};
        for (const auto& message : unicode_strings<Char>()) {
            static auto date = std::chrono::sys_days(std::chrono::year{2050} / 6 / 15);
            static auto int_num = 66;
            static auto dbl_num = 10.0 / 3;

            log->info(
                FormatString<
                    Char,
                    std::add_lvalue_reference_t<decltype(date)>,
                    std::add_lvalue_reference_t<decltype(message)>,
                    std::add_lvalue_reference_t<decltype(int_num)>,
                    std::add_lvalue_reference_t<decltype(dbl_num)>>(FmtMessage.data()),
                date,
                message,
                int_num,
                dbl_num);
            fields.message =
#ifdef SLIMLOG_FMTLIB
                fmt::format(FmtMessage.data(), date, message, int_num, dbl_num);
#else
                std::format(FmtMessage.data(), date, message, int_num, dbl_num);
#endif
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
            expect(cap_out.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });

    // Test multithreaded logger
    _.test("multithreaded", []() {
        StreamCapturer<Char> cap_out;

        const auto message = from_utf8<Char>("Multithreaded test message");

        // Test MultiThreaded logger
        auto log_mt = Logger<String, Char, MultiThreadedPolicy>::create();
        log_mt->template add_sink<OStreamSink>(cap_out);

        log_mt->info(message);
        expect(cap_out.read(), equal_to(message + Char{'\n'}));

        // Verify logger can handle level changes
        log_mt->set_level(Level::Error);
        expect(log_mt->level(), equal_to(Level::Error));

        // Info messages should be filtered out now
        log_mt->info(message);
        expect(cap_out.read(), equal_to(String{}));

        // Error messages should still work
        log_mt->error(message);
        expect(cap_out.read(), equal_to(message + Char{'\n'}));
    });

    // Test logger hierarchy and message propagation
    _.test("logger_hierarchy", []() {
        StreamCapturer<Char> cap_root;
        StreamCapturer<Char> cap_child1;
        StreamCapturer<Char> cap_child2;
        StreamCapturer<Char> cap_grandchild;
        const auto pattern = from_utf8<Char>("[{category}:{level}] {message}");

        // Create logger hierarchy: root -> child1 -> grandchild -> child2
        // Parent relationships are set via constructor
        auto root_log = Logger<String>::create(from_utf8<Char>("root"));
        auto child1_log
            = Logger<String>::create(root_log, from_utf8<Char>("root.child1"), Level::Trace);
        auto child2_log
            = Logger<String>::create(root_log, from_utf8<Char>("root.child2"), Level::Trace);
        auto grandchild_log = Logger<String>::create(
            child1_log, from_utf8<Char>("root.child1.grandchild"), Level::Trace);

        // Add sinks to capture output
        root_log->template add_sink<OStreamSink>(cap_root, pattern);
        child1_log->template add_sink<OStreamSink>(cap_child1, pattern);
        child2_log->template add_sink<OStreamSink>(cap_child2, pattern);
        grandchild_log->template add_sink<OStreamSink>(cap_grandchild, pattern);

        const auto message = from_utf8<Char>("Hierarchy test message");

        grandchild_log->info(message);
        auto expected = from_utf8<Char>("[root.child1.grandchild:INFO] ") + message + Char{'\n'};

        // Grandchild should log to its own sink
        expect(cap_grandchild.read(), equal_to(expected));

        // Message should propagate to child1 (parent)
        expect(cap_child1.read(), equal_to(expected));

        // Message should propagate to root (grandparent)
        expect(cap_root.read(), equal_to(expected));

        // child2 should not receive the message (different branch)
        expect(cap_child2.read(), equal_to(String{}));

        // Test message from child2
        child2_log->warning(message);
        expected = from_utf8<Char>("[root.child2:WARN] ") + message + Char{'\n'};

        // child2 should log to its own sink
        expect(cap_child2.read(), equal_to(expected));

        // Message should propagate to root
        expect(cap_root.read(), equal_to(expected));

        // Other loggers should not receive the message
        expect(cap_child1.read(), equal_to(String{}));
        expect(cap_grandchild.read(), equal_to(String{}));

        // Test level filtering in hierarchy
        grandchild_log->set_level(Level::Error);

        // Debug message should be filtered out at grandchild level
        grandchild_log->debug(message);
        expect(cap_grandchild.read(), equal_to(String{}));
        expect(cap_child1.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(String{}));

        // Reset grandchild level for propagation test
        grandchild_log->set_level(Level::Trace);

        // Test stopping propagation at child1 level
        child1_log->set_propagate(false);
        grandchild_log->warning(message);
        expected = from_utf8<Char>("[root.child1.grandchild:WARN] ") + message + Char{'\n'};

        // Grandchild should still log to its own sink
        expect(cap_grandchild.read(), equal_to(expected));

        // Message should propagate to child1 (immediate parent)
        expect(cap_child1.read(), equal_to(expected));

        // Message should NOT propagate to root (propagation stopped at child1)
        expect(cap_root.read(), equal_to(String{}));

        // child2 should not receive the message (different branch)
        expect(cap_child2.read(), equal_to(String{}));

        // Re-enable propagation for final test
        child1_log->set_propagate(true);

        // Error message should pass through
        grandchild_log->error(message);
        expected = from_utf8<Char>("[root.child1.grandchild:ERROR] ") + message + Char{'\n'};
        expect(cap_grandchild.read(), equal_to(expected));
        expect(cap_child1.read(), equal_to(expected));
        expect(cap_root.read(), equal_to(expected));
    });
});

} // namespace
