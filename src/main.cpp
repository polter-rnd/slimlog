// NOLINTBEGIN

#define ENABLE_MYSTRING 0

#include "format.h"
#include "level.h"
#include "logger.h"
#include "sinks/dummy_sink.h"
#include "sinks/ostream_sink.h"
#if ENABLE_MYSTRING
#include "mystring.h"
#endif
#include "util/locale.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static constexpr wchar_t alphanum_wchar[] = L"0123456789"
                                            L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            L"abcdefghijklmnopqrstuvwxyz"
                                            L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
                                            L"абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
                                            L"!@#$%^&*()-=,./?`~\"' ";

static constexpr char alphanum_char[] = "0123456789"
                                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "!@#$%^&*()-=,./?`~\"' ";

template<typename T>
class null_out_buf : public std::basic_streambuf<T> {
public:
#if defined(__APPLE__)
    using my_int = int;
#else
    using my_int = std::conditional_t<std::is_same_v<T, wchar_t>, unsigned int, int>;
#endif

    virtual std::streamsize xsputn(const T*, std::streamsize n)
    {
        return n;
    }
    virtual my_int overflow(my_int)
    {
        return 1;
    }
};

template<typename T>
class null_out_stream : public std::basic_ostream<T> {
public:
    null_out_stream()
        : std::basic_ostream<T>(&buf)
    {
    }

private:
    null_out_buf<T> buf;
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
    static std::basic_string<T> tmp_s;
#ifndef _MSC_VER
    static auto seed = (unsigned)time(NULL) * getpid();
#else
    static auto seed = (unsigned)time(NULL) * _getpid();
#endif
    tmp_s.clear();
    tmp_s.reserve(len);

    srand(seed++);
    for (int i = 0; i < len; ++i) {
        if constexpr (std::is_same_v<T, wchar_t>) {
            tmp_s += alphanum_wchar[rand() % (sizeof(alphanum_wchar) / sizeof(wchar_t) - 1)];
        } else {
            tmp_s += alphanum_char[rand() % (sizeof(alphanum_char) / sizeof(char) - 1)];
        }
    }

    return std::basic_string_view<T>{tmp_s};
}

double percentile(std::vector<double>& vectorIn, int percent)
{
    auto nth = vectorIn.begin() + (percent * vectorIn.size()) / 100;
    std::nth_element(vectorIn.begin(), nth, vectorIn.end());
    return *nth;
}

void do_test()
{
    namespace Log = SlimLog;

    // auto& devnull = std::wcout;
    null_out_stream<wchar_t> devnull;

    auto log_root = std::make_shared<Log::Logger<std::wstring_view>>(L"main");

    for (int i = 0; i < 50; i++) {
        log_root->add_sink<Log::OStreamSink>(
            devnull,
            L"({category}) [{level}] <{time:%Y-%m-%d %T}> {file}|{line:X}: {message}",
            std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));
    }

    std::vector<std::shared_ptr<Log::Logger<std::wstring_view>>> log_children;
    for (int i = 0; i < 10; i++) {
        auto child = std::make_shared<Log::Logger<std::wstring_view>>(
            L"child_" + std::to_wstring(i), *log_root);
        log_children.push_back(child);

        for (int j = 0; j < 100; j++) {
            child->add_sink<Log::OStreamSink>(
                devnull,
                L">sink_" + std::to_wstring(j)
                    + L"< ({category}) [{level}] <{time:%Y-%m-%d %T}> {file}|{line:X}: "
                      L"{message}");
        }
    }

    std::vector<double> results;

    int k = 0;
    for (const auto& child : log_children) {
        std::vector<std::shared_ptr<Log::Logger<std::wstring_view>>> children2;
        std::shared_ptr<Log::Logger<std::wstring_view>> child2 = child;
        for (int i = 0; i < 100; i++) {
            child2 = std::make_shared<Log::Logger<std::wstring_view>>(
                L"child_new_" + std::to_wstring(i), *child2);
            children2.push_back(child2);

            for (int j = 0; j < 10; j++) {
                child2->add_sink<Log::OStreamSink>(
                    devnull,
                    L">new_sink_" + std::to_wstring(i) + L":" + std::to_wstring(j)
                        + L"< [{level}] <{time:%Y-%m-%d %T}> {file}|{line:X}: {message} "
                          L"({category})");
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

void do_test2()
{
    namespace Log = SlimLog;

    // auto& devnull = std::cout;
    null_out_stream<char> devnull;

    auto log_root = std::make_shared<Log::Logger<std::string_view>>("main");

    for (int i = 0; i < 10; i++) {
        auto child = log_root->add_sink<Log::OStreamSink>(
            devnull,
            "({category}) [{level}] <{time:%Y-%m-%d %T}.{msec}> {file}|{line:X}: "
            "{message}");
    }

    std::vector<double> results;

    for (int k = 0; k < 10; k++) {
        auto res = benchmark([&log_root]() {
            for (int j = 0; j < 10240; j++) {
                int intvar = 123;
                long longvar = 12345678;
                log_root->info(
                    "This is random message {} [{} -> {}]: {}",
                    j,
                    intvar,
                    longvar,
                    gen_random<char>(50));
            }
        });
        std::cout << "[" << k << "] simple message emitting: " << res << "\n";
        results.push_back(res);
    }

    std::cout << "90 perc: " << percentile(results, 90) << "\n";
    std::cout << "median: " << percentile(results, 50) << "\n";
}

namespace SlimLog {
#if ENABLE_MYSTRING
template<typename Char, typename String>
struct ConvertString;

template<typename Char>
struct ConvertString<Char, mystr> {
    std::basic_string_view<Char> operator()(const mystr& str) const
    {
        return std::basic_string_view<char>(str.c_str());
    }
};
#endif
} // namespace SlimLog

auto main(int /*argc*/, char* /*argv*/[]) -> int
{
    namespace Util = SlimLog::Util;
    namespace Log = SlimLog;

#if 1
    try {
        // replace the C++ global locale and the "C" locale with the user-preferred locale
        const Util::Locale::ScopedGlobalLocale myloc("");

        std::cout << "*** This is simple test for the logger ***\n";
        std::cout << "==========================================\n";

#if ENABLE_FMTLIB && defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
        const std::basic_stringstream<char8_t> mystream;
        Log::Logger log19{u8"uchar8 log"};
        log19.add_sink<Log::OStreamSink>(
            mystream, u8"({category}) [{level}] <{time:%Y-%m-%d %T}> {file}|{line}: {message}");
        log19.info(u8"hello from u8!");
        std::cout << std::string_view(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const char*>(mystream.view().data()),
            mystream.view().size());
#endif

        const std::basic_stringstream<char32_t> mystream2;
        Log::Logger log21{U"uchar32 log"};
        log21.add_sink<Log::OStreamSink>(
            mystream2, U"({category}) [{level}] {file}|{line}: {message}");
        log21.info(U"hello from u32!");
        std::wcout << std::wstring_view(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const wchar_t*>(mystream2.view().data()),
            mystream2.view().size());

        auto log_root = std::make_shared<Log::Logger<std::wstring_view>>(L"main");

        auto sink1 = log_root->add_sink<Log::OStreamSink>(
            std::wcout,
            L"!!!!! ({{ sdfsdf {{ ee {category}) erwwer }} ee [{level}] <{time:%Y-%m-%d %T}> "
            L"{function}|{file:*^100}|{line:X}: "
            "{message} "
            "sdf}}{{",
            std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
            std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));

        auto sink2 = log_root->add_sink<Log::OStreamSink>(
            std::wcout,
            L"????? [{level}] <{time:%Y-%m-%d %T}> {function}|{file:*^100}|{line:X}: {message} "
            L"({category})");

        log_root->info(L"Hello {}!", L"World");
        log_root->info(L"Hello {}! ({})", L"World", 2);
        log_root->message(Log::Level::Warning, L"Hello {}! ({})", L"World", 3);
        log_root->info(L"Hello {}! ({})", L"World", 4);

        auto log_child = std::make_shared<Log::Logger<std::wstring_view>>(L"child", *log_root);
        log_child->add_sink<Log::DummySink>();
        log_child->add_sink(sink2);
        log_child->set_sink_enabled(sink2, false);
        log_child->info(L"One two three!");

        auto log_child2 = std::make_shared<Log::Logger<std::wstring_view>>(L"child2", *log_child);
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
        const double t1 = benchmark(do_test2);
        std::cout << "Total elapsed: " << t1 << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }
#else
    /*auto log_root = std::make_shared<Log::Logger<std::wstring_view>>(L"main");
    auto sink1 = log_root->add_sink<Log::OStreamSink>(
        std::wcout,
        L"!!!!! {category} [{level}] <{time:%Y-%m-%d %T}> {thread} "
        L"{file}|{line}|{function}: {message}",
        std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));
    log_root->info(L"Test from root !{}!", L"好");

    std::wcout << L"====================\n";

    auto log_child = std::make_shared<Log::Logger<std::wstring_view>>(L"slave", *log_root);
    auto sink2 = log_child->add_sink<Log::OStreamSink>(
        std::wcout,
        L"????? {category} [{level}] <{time:%Y-%m-%d %T}> {file}|{line}: {message}",
        std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));
    log_child->info(L"Test from slave !好!");

    std::wcout << L"====================\n";

    auto log_superchild
        = std::make_shared<Log::Logger<std::wstring_view>>(L"super_slave", *log_child);
    auto sink3 = log_superchild->add_sink<Log::OStreamSink>(
        std::wcout,
        L"----- {category} [{level}] <{time:%Y-%m-%d %T}> {file}|{line}: {message}",
        std::make_pair(Log::Level::Trace, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Debug, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Warning, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Error, gen_random<wchar_t>(5)),
        std::make_pair(Log::Level::Fatal, gen_random<wchar_t>(5)));
    log_superchild->info(L"Test from super slave !好!");

    std::wcout << L"====================\n";

    log_child.reset();

    log_superchild->info(L"Test 2 from super slave !好!");*/

#endif

#if ENABLE_MYSTRING
    Log::Logger<mystr, char> mylog("mylog");
    mylog.add_sink<Log::OStreamSink>(
        std::cout, "{level}|{time:%Y-%m-%d %X}|{file}:{line}|{function}|{thread} - {message}");
    mystr lal_orig{"kek lal 1"};
    auto lal = std::move(lal_orig);
    mylog.info(lal);
    mylog.info(mystr{"kek lal 2"});
    mylog.info("just string");
    mylog.info([]() { return mystr{"lambda 3"}; });
    mylog.info([]() { return "lambda"; });
    mylog.info([]() { std::cout << "JUST LOG!\n"; });
    mylog.info([](auto& buf) { buf.format("{} {}", "hello", 42); });
#endif
    return 0;
}

// NOLINTEND
