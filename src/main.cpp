// NOLINTBEGIN

#include "log/format.h"
#include "log/level.h"
#include "log/logger.h"
#include "log/sinks/dummy_sink.h"
#include "log/sinks/ostream_sink.h"
#include "util/locale.h"

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

class null_out_buf : public std::basic_streambuf<wchar_t> {
public:
    virtual std::streamsize xsputn(const wchar_t*, std::streamsize n)
    {
        return n;
    }
    virtual unsigned int overflow(unsigned int)
    {
        return 1;
    }
};

class null_out_stream : public std::basic_ostream<wchar_t> {
public:
    null_out_stream()
        : std::basic_ostream<wchar_t>(&buf)
    {
    }

private:
    null_out_buf buf;
};

template<typename Func>
auto benchmark(Func test_func)
{
    const auto start = std::chrono::steady_clock::now();
    test_func();
    const auto stop = std::chrono::steady_clock::now();
    const auto secs = std::chrono::duration<double>(stop - start);
    return secs.count();
}

template<typename T>
std::basic_string_view<T> gen_random(const int len)
{
    static const T alphanum[] = L"0123456789"
                                L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                L"abcdefghijklmnopqrstuvwxyz"
                                L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
                                L"абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
                                L" \t";
    static std::basic_string<T> tmp_s;
    static auto seed = (unsigned)time(NULL) * getpid();
    tmp_s.clear();
    tmp_s.reserve(len);

    srand(seed++);
    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) / sizeof(wchar_t)) - 1];
    }

    return std::basic_string_view<T>{tmp_s};
}

double percentile(std::vector<double>& vectorIn, double percent)
{
    auto nth = vectorIn.begin() + (percent * vectorIn.size()) / 100;
    std::nth_element(vectorIn.begin(), nth, vectorIn.end());
    return *nth;
}

void do_test()
{
    namespace Log = PlainCloud::Log;

    // auto& devnull = std::wcout;
    null_out_stream devnull;

    auto log_root = std::make_shared<Log::Logger<std::wstring_view>>(L"main");

    for (int i = 0; i < 50; i++) {
        log_root->add_sink<Log::OStreamSink>(
            devnull,
            L"({category}) [{level}] {file}|{line:X}: {message:*^100}",
            std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));
    }

    std::vector<std::shared_ptr<Log::Logger<std::wstring_view>>> log_children;
    for (int i = 0; i < 10; i++) {
        auto child = std::make_shared<Log::Logger<std::wstring_view>>(
            L"child_" + std::to_wstring(i), log_root);
        log_children.push_back(child);

        for (int j = 0; j < 100; j++) {
            child->add_sink<Log::OStreamSink>(
                devnull,
                L">sink_" + std::to_wstring(j)
                    + L"< ({category}) [{level}] {file}|{line:X}: {message:*^100}");
        }
    }

    std::vector<double> results;

    int k = 0;
    for (const auto& child : log_children) {
        std::vector<std::shared_ptr<Log::Logger<std::wstring_view>>> children2;
        std::shared_ptr<Log::Logger<std::wstring_view>> child2 = child;
        for (int i = 0; i < 100; i++) {
            child2 = std::make_shared<Log::Logger<std::wstring_view>>(
                L"child_new_" + std::to_wstring(i), child2);
            children2.push_back(child2);

            for (int j = 0; j < 10; j++) {
                child2->add_sink<Log::OStreamSink>(
                    devnull,
                    L">new_sink_" + std::to_wstring(i) + L":" + std::to_wstring(j)
                        + L"< [{level}] {file}|{line:X}: {message:*^100} ({category})");
            }
        }
        auto res = benchmark([&child2]() {
            for (int j = 0; j < 1024; j++) {
                child2->info(L"This is random message {}: {}", j, gen_random<wchar_t>(256 + j));
            }
        });
        std::cout << "[" << k++ << "] message emitting: " << res << "\n";
        results.push_back(res);
    }

    std::cout << "90 perc: " << percentile(results, 90) << "\n";
    std::cout << "median: " << percentile(results, 50) << "\n";
}

auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    namespace Util = PlainCloud::Util;
    namespace Log = PlainCloud::Log;

    try {
        // replace the C++ global locale and the "C" locale with the user-preferred locale
        const Util::Locale::ScopedGlobalLocale myloc("");

        std::cout << "*** This is simple test for the logger ***\n";
        std::cout << "==========================================\n";

#ifdef ENABLE_FMTLIB
        const std::basic_stringstream<char8_t> mystream;
        Log::Logger log19{u8"uchar8 log"};
        log19.add_sink<Log::OStreamSink>(
            mystream, u8"({category}) [{level}] {file}|{line}: {message}");
        log19.info(u8"hello from u8!");
        std::cout << std::string_view(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const char*>(mystream.view().data()),
            mystream.view().size());
#endif

        auto log_root = std::make_shared<Log::Logger<std::wstring_view>>(L"main");

        auto sink1 = log_root->add_sink<Log::OStreamSink>(
            std::wcout,
            L"!!!!! ({{ sdfsdf {{ ee {category:=^20}) erwwer }} ee [{level}] {file}|{line:X}: "
            "{message:*^100} "
            "sdf}}{{",
            std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));

        auto sink2 = log_root->add_sink<Log::OStreamSink>(
            std::wcout, L"????? [{level}] {file}|{line:X}: {message:👆^100} ({category})");

        log_root->info(L"Hello {}!", L"World");
        log_root->info(L"Hello {}! ({})", L"World", 2);
        log_root->message(Log::Level::Warning, L"Hello {}! ({})", L"World", 3);
        log_root->info(L"Hello {}! ({})", L"World", 4);

        auto log_child = std::make_shared<Log::Logger<std::wstring_view>>(L"child", log_root);
        log_child->add_sink<Log::DummySink>();
        log_child->add_sink(sink2);
        log_child->set_sink_enabled(sink2, false);
        log_child->info(L"One two three!");

        auto log_child2 = std::make_shared<Log::Logger<std::wstring_view>>(L"child2", log_child);
        log_child2->info(L"Three two one!");

        log_child->add_sink(sink1);
        log_child->set_sink_enabled(sink1, false);

        log_child2->info(L"Test message!");
        log_child->info(L"Test message2!");
        log_root->info(L"Test from root !好!");
        log_root->info(L"Test from root2 !好!");

        log_root->info([]() { return L"Hello from lambda!"; });
        log_root->info([](auto& fmt) { fmt.format(L"Hello {}!", 123); });
        log_root->message(Log::Level::Info, []() { std::wcout << L"Void lambda\n"; });

        std::cout << "==========================\n";
        const double t1 = benchmark(do_test);
        std::cout << "Total elapsed: " << t1 << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

// NOLINTEND