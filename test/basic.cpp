// SlimLog
#include "slimlog/logger.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"

// Test helpers
#include "helpers/common.h"
#include "helpers/file_capturer.h"
#include "helpers/output_capturer.h"

#include <mettle.hpp>

#include <algorithm>
#include <initializer_list>
#include <sstream>
#include <string>
#include <system_error>

// IWYU pragma: no_include <utility>
// IWYU pragma: no_include <fstream>
// clazy:excludeall=clazy-non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

const suite<char, wchar_t> Basic("basic", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;
    using StringStream = std::basic_ostringstream<Char>;

    // Basic test for all log levels
    _.test("levels", [](Logger<String> log = {}) {
        StringStream oss;
        log.template add_sink<OStreamSink>(oss);
        const auto levels = {
            Level::Trace,
            Level::Debug,
            Level::Info,
            Level::Warning,
            Level::Error,
            Level::Fatal,
        };
        std::for_each(levels.begin(), levels.end(), [&log, &levels, &oss](auto& log_level) {
            log.set_level(log_level);

            for (const auto msg_level : levels) {
                OutputCapturer cap_out(oss);
                log.message(msg_level, test_string<Char>());
                if (msg_level > log_level) {
                    expect(cap_out.read(), equal_to(String{}));
                } else {
                    expect(cap_out.read(), equal_to(test_string<Char>() + Char{'\n'}));
                }
            }
        });
    });

    // NullSink should not output anything
    _.test("null_sink", [](Logger<String> log = {}) {
        auto null_sink = log.template add_sink<NullSink>();
        log.info(test_string<Char>());
        null_sink->flush();
        expect(log.remove_sink(null_sink), equal_to(true));
    });

    // OStreamSink should output to std::cout
    _.test("ostream_sink", [](Logger<String> log = {}) {
        StringStream oss;
        OutputCapturer cap_out{oss};
        auto ostream_sink = log.template add_sink<OStreamSink>(oss);
        log.info(test_string<Char>());
        ostream_sink->flush();
        expect(cap_out.read(), equal_to(test_string<Char>() + Char{'\n'}));
        expect(log.remove_sink(ostream_sink), equal_to(true));
        log.info(test_string<Char>());
        expect(cap_out.read(), equal_to(String{}));
    });

    // FileSink should write to file
    _.test("file_sink", [](Logger<String> log = {}) {
        // Check that invalid path throws an error
        expect([&log]() { log.template add_sink<FileSink>(""); }, thrown<std::system_error>());
        // Check that valid path works
        FileCapturer<Char> cap_file("test_basics.log");
        auto file_sink = log.template add_sink<FileSink>(cap_file.path().string());
        log.info(test_string<Char>());
        file_sink->flush();
        expect(cap_file.read(), equal_to(test_string<Char>() + Char{'\n'}));
    });
});

} // namespace
