#include "log/format.h"
#include "log/level.h"
#include "log/logger.h"
#include "log/policy.h"
#include "log/sinks/dummy_sink.h"
#include "log/sinks/ostream_sink.h"
#include "util.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string_view>
#include <utility>

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
        // std::cout << "testing root syncs: " << i << "\n";
        log_root->add_sink<Log::OStreamSink>(
            devnull,
            L"(%t) [%l] %F|%L: %m",
            std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));

        /*for (int j = 0; j < 1024; j++) {
            log_root->info(L"This is random message {}: {}", j, gen_random<wchar_t>(512));
        }*/
    }

    std::vector<std::shared_ptr<Log::Logger<std::wstring_view>>> log_children;
    for (int i = 0; i < 10; i++) {
        // std::cout << "adding children: " << i << "\n";
        auto child = std::make_shared<Log::Logger<std::wstring_view>>(
            L"child_" + std::to_wstring(i), log_root);
        log_children.push_back(child);

        for (int j = 0; j < 100; j++) {
            child->add_sink<Log::OStreamSink>(
                devnull, L">sink_" + std::to_wstring(j) + L"< (%t) [%l] %F|%L: %m");
        }
        /*for (int j = 0; j < 1024; j++) {
            child->info(L"This is random message {}: {}", j, gen_random<wchar_t>(512));
        }*/
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
                    devnull, L">new_sink_" + std::to_wstring(j) + L"< (%t) [%l] %F|%L: %m");
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

    /*auto log_root = std::make_shared<Log::Logger<std::wstring_view>>(L"main");

    log_root->add_sink<Log::OStreamSink>(
        std::wcout,
        L"!!!!! (%t) [%l] %F|%L: %m",
        std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));

    log_root->add_sink<Log::OStreamSink>(std::wcout, L"????? (%t) [%l] %F|%L: %m");

    log_root->info(L"Hello {}!", L"World");
    log_root->info(L"Hello {}! ({})", L"World", 2);
    log_root->info(L"Hello {}! ({})", L"World", 3);
    log_root->info(L"Hello {}! ({})", L"World", 4);

    return 0;*/
    try {
        // replace the C++ global locale and the "C" locale with the user-preferred locale
        // const Util::ScopedGlobalLocale myloc("");
        const double t1 = benchmark(do_test);
        std::cout << "TOTAL RESULT: " << t1 << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
