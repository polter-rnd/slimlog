// SlimLog
#include "slimlog/logger.h"
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
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <source_location>
#include <string>
#include <system_error>
#include <vector>

// IWYU pragma: no_include <utility>
// IWYU pragma: no_include <fstream>
// clazy:excludeall=clazy-non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

// Mock time function
auto time_mock() -> std::pair<std::chrono::sys_seconds, std::size_t>
{
    // Return 2023-06-15 14:32:45 with 123456 nanoseconds
    constexpr auto Timestamp = 1686839565; // seconds since epoch
    constexpr auto Nanoseconds = 123456; // nanoseconds
    return {std::chrono::sys_seconds{std::chrono::seconds{Timestamp}}, Nanoseconds};
}

const suite<SLIMLOG_CHAR_TYPES> Basic("basic", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;

    // Test empty message
    _.test("empty_message", []() {
        StreamCapturer<Char> cap_out;
        Logger<String> log;
        log.template add_sink<OStreamSink>(cap_out);

        const auto empty_message = String{};
        log.info(empty_message);

        expect(cap_out.read(), equal_to(String{Char{'\n'}}));
    });

    // Test logger categories
    _.test("categories", []() {
        StreamCapturer<Char> cap_out;
        const auto pattern = make_string<Char>("[{category}] {message}");

        const auto default_category = make_string<Char>("default");
        const auto custom_category = make_string<Char>("my_module");

        Logger<String> default_log;
        Logger<String> custom_log{custom_category};

        default_log.template add_sink<OStreamSink>(cap_out, pattern);
        custom_log.template add_sink<OStreamSink>(cap_out, pattern);

        const auto message = make_string<Char>("Test message");

        default_log.info(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[default] ") + message + Char{'\n'}));

        custom_log.info(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[my_module] ") + message + Char{'\n'}));
    });

    // Test convenience logging methods
    _.test("convenience_methods", []() {
        StreamCapturer<Char> cap_out;
        const auto pattern = make_string<Char>("[{level}] {message}");

        Logger<String> log(Level::Trace);
        log.template add_sink<OStreamSink>(cap_out, pattern);

        const auto message = make_string<Char>("Test message");

        log.trace(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[TRACE] ") + message + Char{'\n'}));

        log.debug(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[DEBUG] ") + message + Char{'\n'}));

        log.info(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[INFO] ") + message + Char{'\n'}));

        log.warning(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[WARN] ") + message + Char{'\n'}));

        log.error(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[ERROR] ") + message + Char{'\n'}));

        log.fatal(message);
        expect(cap_out.read(), equal_to(make_string<Char>("[FATAL] ") + message + Char{'\n'}));
    });

    // Test multiple sinks
    _.test("multiple_sinks", []() {
        StreamCapturer<Char> cap_out1;
        StreamCapturer<Char> cap_out2;

        Logger<String> log;
        auto sink1 = log.template add_sink<OStreamSink>(cap_out1);
        auto sink2 = log.template add_sink<OStreamSink>(cap_out2);

        const auto message = make_string<Char>("Multi-sink message");

        log.info(message);

        // Both sinks should receive the same message
        expect(cap_out1.read(), equal_to(message + Char{'\n'}));
        expect(cap_out2.read(), equal_to(message + Char{'\n'}));

        // Remove one sink
        expect(log.remove_sink(sink1), equal_to(true));

        log.info(message);

        // Only second sink should receive the message
        expect(cap_out1.read(), equal_to(String{}));
        expect(cap_out2.read(), equal_to(message + Char{'\n'}));

        // Remove last sink
        expect(log.remove_sink(sink2), equal_to(true));

        log.info(message);

        // No sinks should receive the message now
        expect(cap_out1.read(), equal_to(String{}));
        expect(cap_out2.read(), equal_to(String{}));
    });

    // Test sink management
    _.test("sink_management", []() {
        Logger<String> log;
        StreamCapturer<Char> cap_out;

        // Add sink
        auto sink = log.template add_sink<OStreamSink>(cap_out);
        expect(sink, is_not(nullptr));

        // Remove existing sink
        expect(log.remove_sink(sink), equal_to(true));

        // Try to remove the same sink again
        expect(log.remove_sink(sink), equal_to(false));

        // Try to remove null sink
        expect(log.remove_sink(nullptr), equal_to(false));
    });

    _.test("levels", []() {
        StreamCapturer<Char> cap_out;
        Logger<String> log;
        log.template add_sink<OStreamSink>(cap_out);
        const auto levels = {
            Level::Trace,
            Level::Debug,
            Level::Info,
            Level::Warning,
            Level::Error,
            Level::Fatal,
        };

        std::for_each(levels.begin(), levels.end(), [&log, &levels, &cap_out](auto& log_level) {
            log.set_level(log_level);

            const auto message = make_string<Char>("Hello, World!");
            for (const auto msg_level : levels) {
                log.message(msg_level, message);
                if (msg_level > log_level) {
                    expect(cap_out.read(), equal_to(String{}));
                } else {
                    expect(cap_out.read(), equal_to(message + Char{'\n'}));
                }
            }
        });
    });

    _.test("null_sink", []() {
        Logger<String> log;
        auto null_sink = log.template add_sink<NullSink>();
        log.info(make_string<Char>("Hello, World!"));
        null_sink->flush();
        expect(log.remove_sink(null_sink), equal_to(true));
    });

    _.test("ostream_sink", []() {
        StreamCapturer<Char> cap_out;
        Logger<String> log;

        auto ostream_sink = log.template add_sink<OStreamSink>(cap_out);
        for (const auto& message : unicode_strings<Char>()) {
            log.info(message);
            ostream_sink->flush();
            expect(cap_out.read(), equal_to(message + Char{'\n'}));
        }
    });

    _.test("file_sink", []() {
        Logger<String> log;
        // Check that invalid path throws an error
        expect([&log]() { log.template add_sink<FileSink>(""); }, thrown<std::system_error>());
        // Check that valid path works
        FileCapturer<Char> cap_file("test_basics.log");

        auto file_sink = log.template add_sink<FileSink>(cap_file.path().string());
        for (const auto& message : unicode_strings<Char>()) {
            log.info(message);
            file_sink->flush();
            expect(cap_file.read(), equal_to(message + Char{'\n'}));
        }
    });

    // Basic pattern test
    _.test("pattern", []() {
        FileCapturer<Char> cap_file("test_basics.log");
        const auto pattern = make_string<Char>("({category}) [{level}] "
                                               "<{time:%Y/%d/%m %T} {msec}ms={usec}us={nsec}ns> "
                                               "#{thread} {file}|{line}: {message}");

        Logger<String> log{time_mock};
        auto file_sink = log.template add_sink<FileSink>(cap_file.path().string(), pattern);

        PatternFields<Char> fields;
        fields.category = make_string<Char>("default");
        fields.level = make_string<Char>("INFO");
        fields.thread_id = Util::OS::thread_id();
        fields.file = make_string<Char>(std::source_location::current().file_name());
        fields.time = time_mock().first;
        fields.nsec = time_mock().second;

        for (const auto& message : unicode_strings<Char>()) {
            log.info(message);
            fields.line = std::source_location::current().line() - 1;
            fields.message = message;
            file_sink->flush();
            expect(cap_file.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });

    // Test with simple {time} format (no format specified)
    _.test("simple_time_pattern", []() {
        StreamCapturer<Char> cap_out;

        const auto pattern = make_string<Char>("[{level}] {time} - {message}");
        const auto message = make_string<Char>("Warning message");

        Logger<String> log{time_mock};
        log.template add_sink<OStreamSink>(cap_out, pattern);

        PatternFields<Char> fields;
        fields.level = make_string<Char>("WARN");
        fields.time = time_mock().first;
        fields.nsec = time_mock().second;

        for (const auto& message : unicode_strings<Char>()) {
            log.warning(message);
            fields.message = message;
            expect(cap_out.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });
});

} // namespace
