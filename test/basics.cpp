#include "helpers/file_capturer.h"
#include "helpers/output_capturer.h"
#include "helpers/string_reader.h"

#include <mettle.hpp>
#include <slimlog/logger.h>
#include <slimlog/sinks/file_sink.h>
#include <slimlog/sinks/null_sink.h>
#include <slimlog/sinks/ostream_sink.h>

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

using namespace mettle;
using namespace SlimLog;

const suite<Logger<std::string_view>> Basic("basic", [](auto& _) {
    _.test("null_sink", [](Logger<std::string_view>& log) {
        // NullSink should not output anything
        OutputCapturer cap_out{std::cout};
        auto null_sink = log.add_sink<NullSink>();
        log.info("Hello, World!");
        expect(StringReader(cap_out)(), equal_to(""));
        expect(log.remove_sink(null_sink), equal_to(true));
        log.info("Hello, World!");
        expect(StringReader(cap_out)(), equal_to(""));
    });

    _.test("ostream_sink", [](Logger<std::string_view>& log) {
        // OStreamSink should output to std::cout
        OutputCapturer cap_out{std::cout};
        log.add_sink<OStreamSink>(std::cout);
        log.info("Hello, World!");
        expect(StringReader(cap_out)(), equal_to("Hello, World!\n"));
    });

    _.test("file_sink", [](Logger<std::string_view>& log) {
        // OStreamSink should output to std::cout
        OutputCapturer cap_out{std::cout};
        FileCapturer cap_file("test_basics.log");
        auto file_sink = log.add_sink<FileSink>(cap_file.path().string());
        log.add_sink<OStreamSink>(std::cout);
        log.info("Hello, World!");
        file_sink->flush(); // Flush sink to write to the file
        expect(StringReader(cap_out)(), equal_to("Hello, World!\n"));
        expect(StringReader(cap_file)(), equal_to("Hello, World!\n"));
    });
});

} // namespace
