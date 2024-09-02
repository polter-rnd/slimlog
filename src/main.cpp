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

#ifdef ENABLE_FMTLIB
        const std::basic_stringstream<char8_t> mystream;
        Log::Logger log19{u8"uchar8 log"};
        log19.add_sink<Log::OStreamSink>(mystream, u8"(%t) [%l] %F|%L: %m");
        log19.info(u8"hello from u8!");
        std::cout << std::string_view(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const char*>(mystream.view().data()),
            mystream.view().size())
                  << "\n";
#endif

        auto log_root = std::make_shared<Log::Logger<std::string_view>>("root");

        auto root_sink = log_root->add_sink<Log::OStreamSink>(std::cout, "(%t) [%l] %F|%L: %m");
        log_root->message(Log::Level::Info, "Root!!!");

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
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}