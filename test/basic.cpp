// SlimLog
#include "slimlog/logger.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"
#include "slimlog/util/locale.h"
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
#include <source_location>
#include <string>
#include <system_error>

// IWYU pragma: no_include <utility>
// IWYU pragma: no_include <fstream>
// clazy:excludeall=clazy-non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

// Set global locale for correct Unicode handling
const Util::Locale::ScopedGlobalLocale GLocale{""};

// Mock time function
auto time_mock() -> std::pair<std::chrono::sys_seconds, std::size_t>
{
    // Return 2023-06-15 14:32:45 with 123456 nanoseconds
    constexpr auto Timestamp = 1686839565; // seconds since epoch
    constexpr auto Nanoseconds = 123456; // nanoseconds
    return {std::chrono::sys_seconds{std::chrono::seconds{Timestamp}}, Nanoseconds};
}

// Generate some test messages with different unicode characters
template<typename Char>
auto test_messages() -> std::vector<std::basic_string<Char>>
{
    return {
        make_string<Char>("Simple ASCII message"),
        make_string<Char>("Привет, мир!"),
        make_string<Char>("你好，世界!"),
        make_string<Char>("Some emojis: 😀, 😁, 😂, 🤣, 😃, 😄, 😅, 😆")};
};

const suite<SLIMLOG_CHAR_TYPES> Basic("basic", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;

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
        for (const auto& message : test_messages<Char>()) {
            log.info(message);
            ostream_sink->flush();
            expect(cap_out.read(), equal_to(message + Char{'\n'}));
        }

        expect(log.remove_sink(ostream_sink), equal_to(true));
        log.info(make_string<Char>("Hello, World!"));
        expect(cap_out.read(), equal_to(String{}));
    });

    _.test("file_sink", []() {
        Logger<String> log;
        // Check that invalid path throws an error
        expect([&log]() { log.template add_sink<FileSink>(""); }, thrown<std::system_error>());
        // Check that valid path works
        FileCapturer<Char> cap_file("test_basics.log");

        auto file_sink = log.template add_sink<FileSink>(cap_file.path().string());
        for (const auto& message : test_messages<Char>()) {
            log.info(message);
            file_sink->flush();
            expect(cap_file.read(), equal_to(message + Char{'\n'}));
        }
    });

    // Basic pattern test
    _.test("pattern", []() {
        StreamCapturer<Char> cap_out;
        const auto pattern = make_string<Char>("({category}) [{level}] "
                                               "<{time:%Y/%d/%m %T} {msec}ms={usec}us={nsec}ns> "
                                               "#{thread} {file}|{line}: {message}");

        Logger<String> log{time_mock};
        log.template add_sink<OStreamSink>(cap_out, pattern);

        PatternFields<Char> fields;
        fields.category = make_string<Char>("default");
        fields.level = make_string<Char>("INFO");
        fields.thread_id = Util::OS::thread_id();
        fields.file = make_string<Char>(std::source_location::current().file_name());
        fields.time = time_mock().first;
        fields.nsec = time_mock().second;

        for (const auto& message : test_messages<Char>()) {
            log.info(message);
            const auto log_line = std::source_location::current().line() - 1;
            fields.line = log_line;
            fields.message = message;
            expect(cap_out.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });

    // Additional test with custom pattern
    _.test("custom_pattern", []() {
        StreamCapturer<Char> cap_out;
        const auto pattern = make_string<Char>("[{level}]: {message}");
        const auto message = make_string<Char>("Error message");

        Logger<String> log;
        log.template add_sink<OStreamSink>(cap_out, pattern);

        PatternFields<Char> fields;
        fields.level = make_string<Char>("ERROR");

        for (const auto& message : test_messages<Char>()) {
            log.error(message);
            fields.message = message;
            expect(cap_out.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
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

        for (const auto& message : test_messages<Char>()) {
            log.warning(message);
            fields.message = message;
            expect(cap_out.read(), equal_to(pattern_format<Char>(pattern, fields) + Char{'\n'}));
        }
    });
});

} // namespace
