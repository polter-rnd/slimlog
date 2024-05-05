#include "log/format.h"
#include "log/level.h"
#include "log/logger.h"
#include "log/sinks/ostream_sink.h"
#include "util.h"

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    namespace Log = PlainCloud::Log;
    namespace Util = PlainCloud::Util;

    try {
        // replace the C++ global locale and the "C" locale with the user-preferred locale
        const Util::ScopedGlobalLocale myloc("");

        Log::Logger log("test");
        log.add_sink<Log::OStreamSink>(std::cerr);
        log.info("hello!");

        auto log_root = std::make_shared<Log::Logger<std::string_view>>("kek_root");
        auto root_sink = log_root->add_sink<Log::OStreamSink>(std::cerr);

        const Log::Logger log_child("kek_child", log_root);
        log_child.info("Root sink enabled: {}", "Helloo!!");
        log_root->set_sink_enabled(root_sink, false);
        log_child.info("Root sink disabled!");

        auto my_hdlr = std::make_shared<Log::OStreamSink<std::wstring>>(std::wcerr);
        const Log::Logger log2(std::wstring(L"test"), Log::Level::Info, {my_hdlr});
        log2.info(L"That's from log2!");

        Log::Logger log3("test", Log::Level::Info);
        log3.add_sink<Log::OStreamSink>(std::cerr);

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
        constexpr Log::BasicFormatString<char, std::string_view> Eeee{"Hello {}"};
        log.info(Eeee, std::string_view("pip"));

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log2.info(L"Pipkanoid {}!", 123);
        log2.info(L"Lalka pipka");
        log2.info(L"Привет, {}", L"JOHN REED");
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log2.message(Log::Level::Info, L"Привет, {}", 24);
        log2.message(Log::Level::Info, L"Привет");
        log2.info([]() { return L"Hello from lambda!!!11"; });
        constexpr Log::BasicFormatString<wchar_t, std::wstring_view> Eeee2{L"Hello {}"};
        log2.info(Eeee2, std::wstring_view(L"pip"));
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}