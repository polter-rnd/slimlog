#include "slimlog/format.h"
#include "slimlog/logger.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"

// Test helpers
#include "helpers/common.h"

#include <mettle.hpp>

#include <array>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <latch>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// IWYU pragma: no_include <functional>
// IWYU pragma: no_include <utility>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

// Helper for multithreaded tests
auto run_concurrent_test(int num_threads, int iterations, auto test_func) -> void
{
    std::latch start_latch(num_threads + 1);
    std::vector<std::thread> threads;
    std::atomic<int> operations{0};

    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            start_latch.arrive_and_wait();

            for (int j = 0; j < iterations; ++j) {
                test_func(thread_id, j);
            }

            operations.fetch_add(iterations, std::memory_order_relaxed);
        });
    }

    start_latch.arrive_and_wait();
    for (auto& thread : threads) {
        thread.join();
    }

    expect(operations.load(), equal_to(num_threads * iterations));
};

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
const suite<SLIMLOG_CHAR_TYPES> Multithread("multithread", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string_view<Char>;

    static auto log_filename = get_log_filename<Char>("multithread");
    std::filesystem::remove(log_filename);

    // Test concurrent logging from multiple threads to files and console
    _.test("concurrent_sinks_output", []() {
        constexpr int NumThreads = 8;
        constexpr int IterationsPerThread = 1000;

        auto log = Logger<Char, MultiThreadedPolicy>::create();
        auto file_sink = std::make_shared<FileSink<Char>>(log_filename);
        log->add_sink(file_sink);

        // For char and wchar_t, also log to console
        std::basic_ostream<Char>* out = nullptr;
        if constexpr (std::is_same_v<Char, char>) {
            out = &std::cout;
        } else if constexpr (std::is_same_v<Char, wchar_t>) {
            out = &std::wcout;
        }

        if (out) {
            log->template add_sink<OStreamSink>(*out);
            // NOLINTNEXTLINE(portability-template-virtual-member-function)
            out->setstate(std::ios_base::failbit);
        }

        static constexpr std::array<Char, 21> Message{'T', 'h', 'r', 'e', 'a', 'd', ' ',
                                                      '{', '}', ' ', 'm', 'e', 's', 's',
                                                      'a', 'g', 'e', ' ', '{', '}', '\0'};

        run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
            log->info(FormatString<Char, int&, int&>(Message.data()), thread_id, iteration);

            // Occasionally flush to test concurrent flush operations
            if (iteration % 10 == 0) {
                file_sink->flush();
            }
        });

        if (out) {
            out->clear();
        }

        // Final flush and verify file exists with content
        file_sink->flush();
        expect(std::filesystem::exists(log_filename), equal_to(true));
        expect(std::filesystem::file_size(log_filename), greater(0ULL));
    });

    // Test concurrent sink management
    _.test("concurrent_sink_management", []() {
        constexpr int NumThreads = 4;
        constexpr int IterationsPerThread = 1000;

        auto log = Logger<Char, MultiThreadedPolicy>::create();

        std::atomic<int> total_operations{0};

        run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
            // Create thread-local sinks
            static thread_local auto sink1 = std::make_shared<NullSink<Char>>();
            static thread_local auto sink2 = std::make_shared<NullSink<Char>>();

            // First thread adds sinks, others try to use them
            if (thread_id == 0) {
                log->add_sink(sink1);
                log->add_sink(sink2);
            }

            static constexpr std::array<Char, 24> Message{'s', 'i', 'n', 'k', '1', '=', '{', '}',
                                                          ' ', 's', 'i', 'n', 'k', '2', '=', '{',
                                                          '}', ' ', 'i', 't', '=', '{', '}', '\0'};

            // All threads log through whatever sinks are available
            bool sink1_enabled = log->sink_enabled(sink1);
            bool sink2_enabled = log->sink_enabled(sink2);
            log->info(
                FormatString<Char, bool&, bool&, int&>(Message.data()),
                sink1_enabled,
                sink2_enabled,
                iteration);

            // Last thread removes sinks
            if (thread_id == NumThreads - 1) {
                log->remove_sink(sink1);
                log->remove_sink(sink2);
            }

            total_operations.fetch_add(1, std::memory_order_relaxed);
        });

        // Test passes if we didn't deadlock and operations completed
        expect(total_operations.load(), equal_to(NumThreads * IterationsPerThread));
    });

    // Test concurrent level changes and message filtering
    _.test("concurrent_level_changes", []() {
        constexpr int NumThreads = 4;
        constexpr int IterationsPerThread = 1000;

        auto log = Logger<Char, MultiThreadedPolicy>::create();
        log->template add_sink<NullSink>();

        std::atomic<int> messages_logged{0};

        run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
            // Randomly switch between Debug and Info levels
            log->set_level(iteration % 2 == 0 ? Level::Debug : Level::Info);

            // Try to log at Debug level
            if (log->level_enabled(Level::Debug)) {
                log->debug(from_utf8<Char>("Debug message, thread=" + std::to_string(thread_id)));
                messages_logged.fetch_add(1, std::memory_order_relaxed);
            }

            // Info level should always work
            log->info(from_utf8<Char>("Info message, thread=" + std::to_string(thread_id)));
            messages_logged.fetch_add(1, std::memory_order_relaxed);
        });

        // At least all Info messages should have been logged
        expect(messages_logged.load(), greater_equal(NumThreads * IterationsPerThread));
    });

    // Test logger hierarchy in multithreaded environment
    _.test("concurrent_hierarchy", []() {
        constexpr int NumThreads = 6;
        constexpr int IterationsPerThread = 10000;

        // Create a base hierarchy that will be dynamically modified
        auto root_logger = Logger<Char, MultiThreadedPolicy>::create(from_utf8<Char>("root"));
        auto branch1_logger
            = Logger<Char, MultiThreadedPolicy>::create(root_logger, from_utf8<Char>("branch1"));
        auto branch2_logger
            = Logger<Char, MultiThreadedPolicy>::create(root_logger, from_utf8<Char>("branch2"));

        // Add a sink to root to verify message propagation during hierarchy changes
        root_logger->template add_sink<NullSink>();

        std::atomic<int> total_operations{0};

        run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
            if (thread_id % 3 == 0) {
                // Thread group 1: Create temporary child loggers and attach/detach them
                auto temp_child = Logger<Char, MultiThreadedPolicy>::create(
                    from_utf8<Char>(
                        "temp_" + std::to_string(thread_id) + "_" + std::to_string(iteration)));

                // Randomly attach to different branches
                if (iteration % 2 == 0) {
                    temp_child->set_parent(branch1_logger);
                    temp_child->info(from_utf8<Char>("Message from temp child"));
                    temp_child->set_parent(nullptr); // Detach
                } else {
                    temp_child->set_parent(branch2_logger);
                    temp_child->info(from_utf8<Char>("Message from temp child"));
                    temp_child->set_parent(branch1_logger); // Move between branches
                    temp_child->set_parent(nullptr); // Detach
                }
            } else if (thread_id % 3 == 1) {
                // Thread group 2: Create persistent child loggers and move them around
                static thread_local std::vector<std::shared_ptr<Logger<Char, MultiThreadedPolicy>>>
                    children;

                // Create children for this thread (only once)
                if (children.empty()) {
                    for (int i = 0; i < 3; ++i) {
                        children.push_back(
                            Logger<Char, MultiThreadedPolicy>::create(
                                from_utf8<Char>(
                                    "persistent_" + std::to_string(thread_id) + "_"
                                    + std::to_string(i))));
                    }
                }

                // Move children between branches
                auto& child = children[iteration % children.size()];
                switch (iteration % 4) {
                case 0:
                    child->set_parent(branch1_logger);
                    break;
                case 1:
                    child->set_parent(branch2_logger);
                    break;
                case 2:
                    child->set_parent(root_logger); // Direct to root
                    break;
                default:
                    child->set_parent(nullptr); // Make orphan
                }

                child->info(from_utf8<Char>("Message from persistent child"));
            } else {
                // Thread group 3: Create chains and modify branch relationships

                if (iteration % 20 == 0) {
                    // Swap branch1 and branch2 parents (create cross-links)
                    branch1_logger->set_parent(branch2_logger);
                    branch2_logger->set_parent(root_logger); // Keep branch2 attached to root
                } else if (iteration % 10 == 0) {
                    // Reset to normal structure
                    branch1_logger->set_parent(root_logger);
                    branch2_logger->set_parent(root_logger);
                }

                // Create temporary chain: temp1 -> temp2 -> temp3
                auto temp1 = Logger<Char, MultiThreadedPolicy>::create(
                    from_utf8<Char>("chain1_" + std::to_string(thread_id)));
                auto temp2 = Logger<Char, MultiThreadedPolicy>::create(
                    temp1, from_utf8<Char>("chain2_" + std::to_string(thread_id)));
                auto temp3 = Logger<Char, MultiThreadedPolicy>::create(
                    temp2, from_utf8<Char>("chain3_" + std::to_string(thread_id)));

                // Attach the chain to one of the branches
                temp1->set_parent(iteration % 2 == 0 ? branch1_logger : branch2_logger);

                // Log through the deepest child
                temp3->info(from_utf8<Char>("Message from chain end"));

                // Break the chain in different ways
                switch (iteration % 3) {
                case 0:
                    temp2->set_parent(nullptr); // Break middle link
                    break;
                case 1:
                    temp3->set_parent(temp1); // Skip middle
                    break;
                default:
                    temp1->set_parent(nullptr); // Orphan entire chain
                }
            }

            total_operations.fetch_add(1, std::memory_order_relaxed);

            // Brief yield to encourage interleaving
            if (iteration % 10 == 0) {
                std::this_thread::yield();
            }
        });

        // Test passes if we didn't deadlock and operations completed
        expect(total_operations.load(), equal_to(NumThreads * IterationsPerThread));
    });

    // Test concurrent sink operations and parent changes
    _.test("sink_parent_changes", []() {
        constexpr int NumThreads = 8;
        constexpr int Iterations = 50;
        constexpr int IterationsPerThread = 100;

        for (int iter = 0; iter < Iterations; ++iter) {
            // Create a hierarchy: root -> parent -> child
            auto root_logger = Logger<Char, MultiThreadedPolicy>::create(from_utf8<Char>("root"));
            auto parent_logger
                = Logger<Char, MultiThreadedPolicy>::create(root_logger, from_utf8<Char>("parent"));
            auto child_logger = Logger<Char, MultiThreadedPolicy>::create(
                parent_logger, from_utf8<Char>("child"));

            // Create various sinks
            auto sink1 = std::make_shared<NullSink<Char>>();
            auto sink2 = std::make_shared<NullSink<Char>>();

            std::atomic<int> total_operations{0};

            run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
                if (thread_id % 5 == 0) {
                    // Add/remove sinks from root
                    root_logger->add_sink(sink1);
                    root_logger->remove_sink(sink1);
                } else if (thread_id % 5 == 1) {
                    // Change parent relationships back and forth
                    child_logger->set_parent(root_logger);
                    child_logger->set_parent(parent_logger);
                } else if (thread_id % 5 == 2) {
                    // Add/remove sinks from parent while changing child's parent
                    parent_logger->add_sink(sink2);
                    child_logger->set_parent(nullptr);
                    parent_logger->remove_sink(sink2);
                    child_logger->set_parent(parent_logger);
                } else if (thread_id % 5 == 3) {
                    // Create new child loggers with the parent (constructor path)
                    auto new_child = Logger<Char, MultiThreadedPolicy>::create(
                        parent_logger,
                        from_utf8<Char>(
                            "new_child_" + std::to_string(thread_id) + "_"
                            + std::to_string(iteration)));
                } else {
                    if (iteration % 2 == 0) {
                        // Create and immediately change parent
                        auto temp_child = Logger<Char, MultiThreadedPolicy>::create(
                            from_utf8<Char>(
                                "temp_child_" + std::to_string(thread_id) + "_"
                                + std::to_string(iteration)));
                        temp_child->set_parent(parent_logger);
                    } else {
                        // Change parent of existing child logger (set_parent path)
                        child_logger->set_parent(root_logger);
                        child_logger->set_parent(parent_logger); // Back to original
                    }
                }

                total_operations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield(); // Encourage context switching
            });

            // Test passes if we didn't deadlock and operations completed
            expect(total_operations.load(), equal_to(NumThreads * IterationsPerThread));
        }
    });

    // Test chain parent relationship changes
    _.test("parent_chain_relationships", []() {
        constexpr int Iterations = 20;
        constexpr int NumThreads = 10;
        constexpr int IterationsPerThread = 100;

        for (int iter = 0; iter < Iterations; ++iter) {
            // Create multiple loggers that will be rearranged
            std::vector<std::shared_ptr<Logger<Char, MultiThreadedPolicy>>> loggers;
            loggers.reserve(6);
            for (int i = 0; i < 6; ++i) {
                loggers.push_back(
                    Logger<Char, MultiThreadedPolicy>::create(
                        from_utf8<Char>("logger_" + std::to_string(i))));
            }

            std::atomic<int> total_operations{0};

            run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
                // Choose unique parent and child indices to avoid self-assignment
                const int parent_idx = thread_id % loggers.size();
                const int child_idx
                    = (parent_idx + 1 + (iteration % (loggers.size() - 1))) % loggers.size();

                // Now parent_idx and child_idx are guaranteed to be different
                loggers[child_idx]->set_parent(loggers[parent_idx]);
                loggers[child_idx]->set_parent(nullptr); // Clear

                // Create chain relationships every 3rd iteration
                if (iteration % 3 == 0) {
                    // Choose three unique logger indices
                    const int base = thread_id % loggers.size();
                    const int logger1 = base;
                    const int logger2 = (base + 1) % loggers.size();
                    const int logger3 = (base + 2) % loggers.size();

                    // Create chain: logger1 -> logger2 -> logger3
                    loggers[logger2]->set_parent(loggers[logger1]);
                    loggers[logger3]->set_parent(loggers[logger2]);
                    // Break chain
                    loggers[logger3]->set_parent(nullptr);
                    loggers[logger2]->set_parent(nullptr);
                }

                total_operations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield(); // Encourage interleaving
            });

            // Test passes if we didn't deadlock and operations completed
            expect(total_operations.load(), equal_to(NumThreads * IterationsPerThread));
        }
    });

    // Comprehensive stress test for all threading scenarios
    _.test("stress_test", []() {
        constexpr int Iterations = 10;
        constexpr int NumThreads = 12;
        constexpr int IterationsPerThread = 100;
        constexpr int ChildLoggers = 4;

        for (int iter = 0; iter < Iterations; ++iter) {
            // Create a moderately complex hierarchy
            auto root = Logger<Char, MultiThreadedPolicy>::create(from_utf8<Char>("root"));

            std::vector<std::shared_ptr<Logger<Char, MultiThreadedPolicy>>> loggers;
            loggers.push_back(root);

            // Create some initial children
            for (int i = 0; i < ChildLoggers; ++i) {
                auto logger = Logger<Char, MultiThreadedPolicy>::create(
                    root, from_utf8<Char>("child_" + std::to_string(i)));
                loggers.push_back(logger);
            }

            // Create sinks
            std::vector<std::shared_ptr<NullSink<Char>>> sinks;
            sinks.reserve(3);
            for (int i = 0; i < 3; ++i) {
                sinks.push_back(std::make_shared<NullSink<Char>>());
            }

            std::atomic<int> total_operations{0};

            run_concurrent_test(NumThreads, IterationsPerThread, [&](int thread_id, int iteration) {
                std::mt19937 rng((thread_id * IterationsPerThread) + iteration);
                switch (rng() % 7) {
                case 0: {
                    // Create new logger with random parent
                    const int parent_idx = rng() % loggers.size();
                    auto new_logger = Logger<Char, MultiThreadedPolicy>::create(
                        loggers[parent_idx],
                        from_utf8<Char>(
                            "dynamic_" + std::to_string(thread_id) + "_"
                            + std::to_string(iteration)));
                    // Don't store it to allow destruction
                    break;
                }
                case 1: {
                    // Change parent relationship
                    const int child_idx = (rng() % (loggers.size() - 1)) + 1; // Skip root
                    const int parent_idx = rng() % loggers.size();
                    if (child_idx != parent_idx) {
                        loggers[child_idx]->set_parent(loggers[parent_idx]);
                    }
                    break;
                }
                case 2: {
                    // Add/remove sinks
                    const int logger_idx = rng() % loggers.size();
                    const int sink_idx = rng() % sinks.size();
                    if (rng() % 2) {
                        loggers[logger_idx]->add_sink(sinks[sink_idx]);
                    } else {
                        loggers[logger_idx]->remove_sink(sinks[sink_idx]);
                    }
                    break;
                }
                case 3: {
                    // Toggle sink enabled/disabled
                    const int logger_idx = rng() % loggers.size();
                    const int sink_idx = rng() % sinks.size();
                    loggers[logger_idx]->set_sink_enabled(sinks[sink_idx], rng() % 2);
                    break;
                }
                case 4: {
                    // Toggle propagation
                    const int logger_idx = rng() % loggers.size();
                    loggers[logger_idx]->set_propagate(rng() % 2);
                    break;
                }
                case 5: {
                    // Change level
                    const int logger_idx = rng() % loggers.size();
                    const Level levels[]
                        = {Level::Debug, Level::Info, Level::Warning, Level::Error};
                    loggers[logger_idx]->set_level(levels[rng() % 4]);
                    break;
                }
                case 6: {
                    // Clear parent (make orphan)
                    const int child_idx = (rng() % (loggers.size() - 1)) + 1; // Skip root
                    loggers[child_idx]->set_parent(nullptr);
                    break;
                }
                default:
                    // Should not reach here
                    break;
                }

                total_operations.fetch_add(1, std::memory_order_relaxed);

                // Brief yield to encourage interleaving every few operations
                if (iteration % 5 == 0) {
                    std::this_thread::yield();
                }
            });

            // Test passes if we completed many operations without deadlock/crash
            expect(total_operations.load(), equal_to(NumThreads * IterationsPerThread));
        }
    });
});

} // namespace
