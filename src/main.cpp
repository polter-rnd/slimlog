#include "log/logger.h" // for Logger
#include "log/sinks/console_sink.h" // for ConsoleSink
#include "util.h" // for Util

#include <exception> // for exception
#include <iostream> // for char_traits, operator<<, basic_o...
#include <string_view> // for string_view

auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    namespace Log = PlainCloud::Log;
    namespace Util = PlainCloud::Util;

    try {
        // replace the C++ global locale and the "C" locale with the user-preferred locale
        const Util::ScopedGlobalLocale myloc("");

        Log::Logger log("test");
        log.add_sink<Log::ConsoleSink>();

        /*auto log_root = std::make_shared<Log::Logger<std::string_view>>("kek_root");
        auto root_sink = log_root->add_sink<Log::ConsoleSink>();

        Log::Logger log_child("kek_child", log_root);
        log_child.info("Root sink enabled: {}", 123);
        log_root->set_sink_enabled(root_sink, false);
        log_child.info("Root sink disabled!");*/
        return 0;

        Log::Sink<std::wstring>* dd = new Log::ConsoleSink<std::wstring>();
        // auto my_hdlr = std::make_shared<Log::ConsoleSink<std::wstring>>();
        // const Log::Logger log2(std::wstring(L"test"), Log::Level::Info, {my_hdlr});
        //  log2.info(L"That's from log2!");
#if 0
        Log::Logger log3("test", Log::Level::Info);
        log3.add_sink<Log::ConsoleSink>();

        log3.info("Hello!");
        log3.info("Hello {}", "world");
        log3.info([]() { return "Hello from lambda!!!11"; });

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        log.emit(Log::Level::Warning, "dsfsdf {}", 1234);
        // log.emit(Log::Level::Warning, U"SUPER INICODE");
        log.emit(Log::Level::Fatal, "hi hi hi");
        const char* kkkk = "My test format: {}";
        log.emit(Log::Level::Info, kkkk);
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
        log2.emit(Log::Level::Info, L"Привет, {}", 24);
        log2.emit(Log::Level::Info, L"Привет");
        log2.info([]() { return L"Hello from lambda!!!11"; });
        constexpr Log::BasicFormatString<wchar_t, std::wstring_view> Eeee2{L"Hello {}"};
        log2.info(Eeee2, std::wstring_view(L"pip"));
#endif
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}