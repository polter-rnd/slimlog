#include "helpers/file_capturer.h"
#include "helpers/output_capturer.h"

#include <mettle.hpp>
#include <slimlog/logger.h>
#include <slimlog/sinks/file_sink.h>
#include <slimlog/sinks/null_sink.h>
#include <slimlog/sinks/ostream_sink.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

// IWYU pragma: no_include <utility>
// IWYU pragma: no_include <functional>
// IWYU pragma: no_include <memory>
// clazy:excludeall=clazy-non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

const suite<Logger<std::string_view>> Basic("basic", [](auto& _) {
    _.test("null_sink", [](Logger<std::string_view>& log) {
        // NullSink should not output anything
        OutputCapturer cap_out{std::cout};
        auto null_sink = log.add_sink<NullSink>();
        log.info("Hello, World!");
        expect(cap_out.read(), equal_to(""));
        expect(log.remove_sink(null_sink), equal_to(true));
        log.info("Hello, World!");
        expect(cap_out.read(), equal_to(""));
    });

    _.test("ostream_sink", [](Logger<std::string_view>& log) {
        // OStreamSink should output to std::cout
        OutputCapturer cap_out{std::cout};
        log.add_sink<OStreamSink>(std::cout);
        log.info("Hello, World!");
        expect(cap_out.read(), equal_to("Hello, World!\n"));
    });

    _.test("file_sink", [](Logger<std::string_view>& log) {
        // OStreamSink should output to std::cout
        OutputCapturer cap_out{std::cout};
        FileCapturer cap_file("test_basics.log");
        auto file_sink = log.add_sink<FileSink>(cap_file.path().string());
        log.add_sink<OStreamSink>(std::cout);
        log.info("Hello, World!");
        file_sink->flush(); // Flush sink to write to the file
        expect(cap_out.read(), equal_to("Hello, World!\n"));
        expect(cap_file.read(), equal_to("Hello, World!\n"));
    });
});

} // namespace
