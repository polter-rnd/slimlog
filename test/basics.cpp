#include "helpers/file_reader.h"
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
        expect(capture.str().empty());

        // OStreamSink should output to std::cout
        log.add_sink<OStreamSink>(std::cout);
        log.info("Hello, World!");
        expect(StringReader(capture) == "Hello, World!\n");

        // FileSink should output to a file
        std::filesystem::path log_path = std::filesystem::current_path() / "slimlog_test.log";
        const std::unique_ptr<std::filesystem::path, void (*)(std::filesystem::path*)> path_guard
            = {&log_path, [](std::filesystem::path* path) { std::filesystem::remove(*path); }};
        auto file_sink = log.add_sink<FileSink>(log_path.string());
        log.info("Hello, World!");
        log.remove_sink(file_sink); // Remove sink to decrease reference counter
        file_sink.reset(); // Delete sink to close the file
        expect(FileReader(log_path) == "Hello, World!\n");
        expect(StringReader(capture) == "Hello, World!\n");
    };
}
