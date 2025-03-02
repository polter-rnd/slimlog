#include "file_reader.h"
#include "output_capturer.h"

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

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    using namespace SlimLog;
    using namespace boost::ut;

    "simple"_test = [] {
        OutputCapturer capture{std::cout};
        Logger log("main");

        // NullSink should not output anything
        log.add_sink<NullSink>();
        log.info("Hello, World!");
        expect(capture.output().empty());

        // OStreamSink should output to std::cout
        log.add_sink<OStreamSink>(std::cout);
        log.info("Hello, World!");
        expect(capture.output() == "Hello, World!\n");
        capture.clear();

        // FileSink should output to a file
        const auto log_path = std::filesystem::current_path() / "slimlog_test.log";
        auto file_sink = log.add_sink<FileSink>(log_path.string());
        log.info("Hello, World!");
        log.remove_sink(file_sink); // Remove sink to decrease reference counter
        file_sink.reset(); // Delete sink to close the file
        expect(FileReader(log_path) == "Hello, World!\n");
        expect(capture.output() == "Hello, World!\n");
        std::filesystem::remove(log_path);
    };
}
