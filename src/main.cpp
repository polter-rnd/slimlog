#include <log/logger.h>
#include <log/sinks/console_sink.h>

#include <memory>

using namespace plaincloud;

struct scoped_locale_global {
    scoped_locale_global(const std::locale& locale)
        : m_old_locale(std::locale::global(locale))
    {
    }

    ~scoped_locale_global()
    {
        std::locale::global(m_old_locale);
    }

private:
    std::locale m_old_locale;
};

int main(int argc, char* argv[])
{
    // Log::Logger::setGlobalLevel(Log::Level::Trace);q
    //  Log::Logger::addGlobalHandler(hndlr);
    Log::Logger log("test");
    log.add_sink<Log::ConsoleSink>();

    auto myHdlr = std::make_shared<Log::ConsoleSink<std::string>>();
    Log::Logger log2(std::string("test"), Log::Level::Info, {myHdlr});
    log2.info("That's from log2!");

    // replace the C++ global locale and the "C" locale with the user-preferred locale
    // scoped_locale_global loc(std::locale(""));
    // use the new global locale for future wide character output

    Log::Logger log3("test", Log::Level::Info);
    log3.add_sink<Log::ConsoleSink>();

    log3.info("Hello!");
    log3.info("Hello {}", "world");
    log3.info([]() { return "Hello from lambda!!!11"; });

    log.emit(Log::Level::Warning, "dsfsdf {}", 1234);
    // log.emit(Log::Level::Warning, U"SUPER INICODE");
    log.emit(Log::Level::Fatal, "hi hi hi");
    const char* kk = "My test format: {}";
    log.emit(Log::Level::Info, kk);
    log.info("Pipkanoid {}!", 123);
    log.info("Lalka pipka");

    /*log2.info(L"Pipkanoid {}!", 123);
    log2.info(L"Lalka pipka");
    log2.info(L"Привет, {}", 24);*/

    return 0;
}