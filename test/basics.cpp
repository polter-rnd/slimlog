#include "helpers/file_capturer.h"
#include "helpers/output_capturer.h"
#include "helpers/string_reader.h"

#include <boost/ut.hpp>
#include <slimlog/logger.h>
#include <slimlog/sinks/file_sink.h>
#include <slimlog/sinks/null_sink.h>
#include <slimlog/sinks/ostream_sink.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <iostream>

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    using namespace boost::ut;

    bool ok = true;
    FileCapturer cap_file("test_basics.log");
    OutputCapturer cap_out{std::cout};

    "simple"_test = [&cap_file, &cap_out, &ok] () {
        SlimLog::Logger log("main");

        // NullSink should not output anything
        log.add_sink<SlimLog::NullSink>();
        log.info("Hello, World!");
        ok &= expect(cap_out.str().empty());

        // OStreamSink should output to std::cout
        log.add_sink<SlimLog::OStreamSink>(std::cout);
        log.info("Hello, World!");
        ok &= expect(StringReader(cap_out) == "Hello, World!\n");

        // FileSink should output to a file
        auto file_sink = log.add_sink<SlimLog::FileSink>(cap_file.path().string());
        log.info("Hello, World!");
        file_sink->flush(); // Flush sink to write to the file
        ok &= expect(StringReader(cap_file) == "Hello, World!\n");
        ok &= expect(StringReader(cap_out) == "Hello, World!\n");
        log.info("Hello, World2!");
        file_sink->flush(); // Flush sink to write to the file
        ok &= expect(StringReader(cap_file) == "Hello, World2!\n");
        ok &= expect(StringReader(cap_out) == "Hello, World2!\n");
        log.remove_sink(file_sink); // Remove sink to decrease reference counter
    };

    // Remove file on success
    if (ok) {
        cap_file.remove_file();
    }
}
