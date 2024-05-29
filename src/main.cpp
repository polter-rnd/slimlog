#include "log/format.h"
#include "log/level.h"
#include "log/logger.h"
#include "log/policy.h"
#include "log/sinks/ostream_sink.h"
#include "util.h"

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>

static constexpr auto SomeConstant = 1234;

auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    namespace Log = PlainCloud::Log;
    namespace Util = PlainCloud::Util;

    try {
        // replace the C++ global locale and the "C" locale with the user-preferred locale
        const Util::ScopedGlobalLocale myloc("");
        const char* kektest = "test";
        Log::Logger log{kektest};
        log.add_sink<Log::OStreamSink>(std::cerr);
        log.info("hello!");

        const std::basic_stringstream<char8_t> mystream;
        Log::Logger log19{u8"uchar8 log"};
        log19.add_sink<Log::OStreamSink>(mystream, u8"(%t) [%l] %F|%L: %m");
        log19.info(u8"hello from u8!");
        std::cout << std::string_view(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const char*>(mystream.view().data()),
            mystream.view().size())
                  << "\n";

        auto log_root = std::make_shared<Log::Logger<std::string_view>>("kek_root");

        auto root_sink = log_root->add_sink<Log::OStreamSink>(
            std::cout,
            "(%t) [%l] %F|%L: %m",
            std::make_pair(Log::Level::Trace, "trc"),
            std::make_pair(Log::Level::Debug, "dbg"),
            std::make_pair(Log::Level::Warning, "wrn"),
            std::make_pair(Log::Level::Error, "err"),
            std::make_pair(Log::Level::Fatal, "ftl"));
        log_root->message(Log::Level::Info, "Root!!!");

        // log_root->info([](int dd) { return "123"; });

        Log::Logger log_child("kek_child", Log::Level::Info);
        log_child.add_sink(root_sink);
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_child.info("Root sink enabled: {}", "Helloo!!");

        log_root->set_sink_enabled(root_sink, false);
        log_child.info("Root sink disabled!");

        Log::Logger<std::wstring_view, wchar_t, Log::SingleThreadedPolicy> log2(
            L"test", Log::Level::Info);
        auto my_hdlr
            = log2.add_sink<Log::OStreamSink>(std::wcerr, L"W (%t) [%l] %F|%L: %m %o sdf%");
        log2.info(L"That's from log2!");

        Log::Logger log3("test", Log::Level::Info);
        log3.add_sink<Log::OStreamSink>(std::cerr)
            ->set_pattern(">>>>> %l %F %m")
            ->set_levels({{Log::Level::Info, "KEK"}});

        log3.info("Hello!");
        log3.info("Hello {}", "world");
        log3.info([]() { return "Hello from lambda!!!11"; });

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log.message(Log::Level::Warning, "dsfsdf {}", 1234);
        // log.message(Log::Level::Warning, U"SUPER INICODE");
        log.message(Log::Level::Fatal, "hi hi hi");
        const char* kkkk = "My test format: {}";
        log.message(Log::Level::Info, kkkk);
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log.info("Pipkanoid {}!", 123);
        log.info("Lalka pipka");
        constexpr Log::FormatString<char, std::string_view> Eeee{"Hello {}"};
        log.info(Eeee, std::string_view("pip"));

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log2.info(L"Pipkanoid {}!", 123);

        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");
        log2.info(L"Pipkanoid!");

        log2.info(L"Lalka pipka");
        log2.info(L"Привет, {}", L"JOHN REED");
        log2.info(L"Привет, {}", L"JOHN REED");
        log2.info(L"Привет, {}", L"JOHN REED");
        log2.info(L"Привет, {}", L"JOHN REED");
        log2.info(L"Привет, {}", L"JOHN REED");
        log2.info(L"Привет, {}", L"JOHN REED");
        log2.info(L"Привет, {}", L"JOHN REED");

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log2.message(Log::Level::Info, L"Привет, {}", 24);
        log2.message(Log::Level::Info, L"Привет");
        // log2.info([]() { return L"Hello from lambda!!!11"; });
        constexpr Log::FormatString<wchar_t, std::wstring_view> Eeee2{L"Hello {}"};
        log2.info(Eeee2, std::wstring_view(L"pip"));

        log2.info([](auto& buf) { buf.format(L"KEKEKEKEKE {}", SomeConstant); });

        const std::basic_stringstream<wchar_t> keklal;
        Log::Logger log9{L"logger"};
        log9.add_sink<Log::OStreamSink>(keklal)->set_pattern(L"(%t) [%l] %F|%L: %m");
        log9.info(L"Ой лол 😃");
        log9.info(L"Hello guys!!!");
        std::wcout << keklal.view();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}