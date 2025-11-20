#include "slimlog/format.h"
#include "slimlog/logger.h"
#include "slimlog/sinks/file_sink.h"
#include "slimlog/sinks/null_sink.h"
#include "slimlog/sinks/ostream_sink.h"

// Test helpers
#include "helpers/common.h"
#include "helpers/file_capturer.h"
#include "helpers/stream_capturer.h"

#include <mettle.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <latch>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace mettle;
using namespace SlimLog;

const suite<char> Multithread("multithread", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;

    static auto log_filename = get_log_filename<Char>("multithread");
    std::filesystem::remove(log_filename);

    // Test concurrent logging from multiple threads
    _.test("concurrent_logging", []() {
        constexpr int num_threads = 8;
        constexpr int messages_per_thread = 1000;

        // StreamCapturer<Char> cap_out;
        Logger<String, Char, MultiThreadedPolicy> log;
        log.template add_sink<OStreamSink>(std::cout);

        std::latch start_latch(num_threads + 1);
        std::vector<std::thread> threads;
        std::atomic<int> total_messages{0};

        // Launch threads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i]() {
                start_latch.arrive_and_wait();

                for (int j = 0; j < messages_per_thread; ++j) {
                    const auto msg = from_utf8<Char>(
                        "Thread " + std::to_string(thread_id) + " message " + std::to_string(j));
                    log.info(msg);
                    total_messages.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        // Temporary disable console output for test
        std::cout.setstate(std::ios_base::failbit);

        // Start all threads simultaneously
        start_latch.arrive_and_wait();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Restore console output
        std::cout.clear();

        // Verify total number of messages
        expect(total_messages.load(), equal_to(num_threads * messages_per_thread));
    });

    // Test concurrent sink management
    _.test("concurrent_sink_management", []() {
        constexpr int num_threads = 4;
        Logger<String, Char, MultiThreadedPolicy> log;

        std::latch start_latch(num_threads + 1);
        std::vector<std::thread> threads;

        // Launch threads that will add and remove sinks concurrently
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                // Create thread-local sinks
                auto sink1 = std::make_shared<NullSink<String>>();
                auto sink2 = std::make_shared<NullSink<String>>();
                start_latch.arrive_and_wait();

                for (int j = 0; j < 100; ++j) {
                    // First thread adds sinks, others try to use them
                    if (i == 0) {
                        log.add_sink(sink1);
                        log.add_sink(sink2);
                    }

                    // All threads log through whatever sinks are available
                    log.info(from_utf8<Char>("Test message"));

                    // Last thread removes sinks
                    if (i == num_threads - 1) {
                        log.remove_sink(sink1);
                        log.remove_sink(sink2);
                    }
                }
            });
        }

        // Start all threads simultaneously
        start_latch.arrive_and_wait();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    });

    // Test concurrent level changes and message filtering
    _.test("concurrent_level_changes", []() {
        constexpr int num_threads = 4;
        Logger<String, Char, MultiThreadedPolicy> log;
        log.template add_sink<NullSink>();

        std::latch start_latch(num_threads + 1);
        std::vector<std::thread> threads;
        std::atomic<int> filtered_messages{0};

        // Launch threads that will change levels and log messages concurrently
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&]() {
                start_latch.arrive_and_wait();

                for (int j = 0; j < 1000; ++j) {
                    // Randomly switch between Debug and Info levels
                    log.set_level(j % 2 == 0 ? Level::Debug : Level::Info);

                    // Try to log at Debug level
                    if (log.level_enabled(Level::Debug)) {
                        log.debug(from_utf8<Char>("Debug message"));
                        filtered_messages.fetch_add(1, std::memory_order_relaxed);
                    }

                    // Info level should always work
                    log.info(from_utf8<Char>("Info message"));
                    filtered_messages.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        // Start all threads simultaneously
        start_latch.arrive_and_wait();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // At least all Info messages should have been logged
        expect(filtered_messages.load(), greater_equal(num_threads * 1000));
    });

    // Test concurrent file logging
    _.test("concurrent_file_logging", []() {
        constexpr int num_threads = 8;
        constexpr int messages_per_thread = 100;

        Logger<String, Char, MultiThreadedPolicy> log;
        auto sink = std::make_shared<FileSink<String>>(log_filename);
        log.add_sink(sink);

        std::latch start_latch(num_threads + 1);
        std::vector<std::thread> threads;

        // Launch threads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i]() {
                start_latch.arrive_and_wait();

                for (int j = 0; j < messages_per_thread; ++j) {
                    const auto msg = from_utf8<Char>(
                        "Thread " + std::to_string(thread_id) + " message " + std::to_string(j));
                    log.info(msg);

                    // Occasionally flush to test concurrent flush operations
                    if (j % 10 == 0) {
                        sink->flush();
                    }
                }
            });
        }

        // Start all threads simultaneously
        start_latch.arrive_and_wait();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Final flush and close
        sink->flush();

        // Verify file exists and has content
        expect(std::filesystem::exists(log_filename), equal_to(true));
        expect(std::filesystem::file_size(log_filename), greater(0ULL));
    });

    // Test logger hierarchy in multithreaded environment
    _.test("concurrent_hierarchy", []() {
        constexpr int num_threads = 4;
        constexpr int num_children = 4;

        auto root_logger
            = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(from_utf8<Char>("root"));
        std::vector<std::shared_ptr<Logger<String, Char, MultiThreadedPolicy>>> child_loggers;

        // Create child loggers
        for (int i = 0; i < num_children; ++i) {
            auto child = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                from_utf8<Char>("child" + std::to_string(i)));
            child->set_parent(root_logger);
            child_loggers.push_back(std::move(child));
        }

        root_logger->template add_sink<NullSink>();

        std::latch start_latch(num_threads + 1);
        std::vector<std::thread> threads;
        std::atomic<int> total_messages{0};

        // Launch threads that will log through different loggers concurrently
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, thread_id = i]() {
                start_latch.arrive_and_wait();

                for (int j = 0; j < 100; ++j) {
                    // Log through a random child logger
                    const auto logger_idx = (thread_id + j) % num_children;
                    const auto msg = from_utf8<Char>(
                        "Thread " + std::to_string(thread_id) + " message " + std::to_string(j));
                    child_loggers[logger_idx]->info(msg);
                    total_messages.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        // Start all threads simultaneously
        start_latch.arrive_and_wait();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Verify total number of messages
        expect(total_messages.load(), equal_to(num_threads * 100));
    });

    // Test concurrent parent assignment and child construction (deadlock prevention)
    _.test("parent_assignment", []() {
        constexpr int num_iterations = 50;
        constexpr int num_threads = 8;

        for (int iter = 0; iter < num_iterations; ++iter) {
            auto parent_logger = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                from_utf8<Char>("parent"));

            // Create a child that we'll use for set_parent operations
            auto existing_child = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                from_utf8<Char>("existing_child"));

            std::latch start_latch(num_threads + 1);
            std::vector<std::thread> threads;
            std::atomic<int> successful_operations{0};
            std::atomic<bool> should_stop{false};

            // Launch threads that will create children and change parent relationships concurrently
            for (int i = 0; i < num_threads; ++i) {
                threads.emplace_back([&, thread_id = i]() {
                    start_latch.arrive_and_wait();

                    int local_operations = 0;
                    while (!should_stop.load(std::memory_order_relaxed) && local_operations < 20) {
                        if (thread_id % 3 == 0) {
                            // Create new child loggers with the parent (constructor path)
                            // This exercises address-based lock ordering in constructor
                            auto child
                                = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                                    parent_logger,
                                    from_utf8<Char>(
                                        "child_" + std::to_string(thread_id) + "_"
                                        + std::to_string(local_operations)));
                            ++local_operations;
                        } else if (thread_id % 3 == 1) {
                            // Change parent of existing child (set_parent path)
                            // This exercises address-based lock ordering in set_parent
                            existing_child->set_parent(parent_logger);
                            existing_child->set_parent(nullptr); // Clear parent
                            ++local_operations;
                        } else {
                            // Create and immediately change parent
                            // This exercises both constructor and set_parent paths
                            auto temp_child
                                = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                                    from_utf8<Char>(
                                        "temp_child_" + std::to_string(thread_id) + "_"
                                        + std::to_string(local_operations)));
                            temp_child->set_parent(parent_logger);
                            ++local_operations;
                        }

                        // Small yield to encourage thread interleaving
                        std::this_thread::yield();
                    }
                    successful_operations.fetch_add(local_operations, std::memory_order_relaxed);
                });
            }

            // Start all threads simultaneously
            start_latch.arrive_and_wait();

            // Let them run for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            should_stop.store(true, std::memory_order_relaxed);

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Test passes if we didn't deadlock (threads completed)
            expect(successful_operations.load(), greater(0));
        }
    });

    // Test concurrent sink operations and parent changes (update_propagated_sinks deadlock
    // prevention)
    _.test("sink_parent_changes", []() {
        constexpr int num_iterations = 30;
        constexpr int num_threads = 6;

        for (int iter = 0; iter < num_iterations; ++iter) {
            // Create a hierarchy: root -> parent -> child
            auto root_logger = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                from_utf8<Char>("root"));
            auto parent_logger = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                root_logger, from_utf8<Char>("parent"));
            auto child_logger = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                parent_logger, from_utf8<Char>("child"));

            // Create various sinks
            auto sink1 = std::make_shared<NullSink<String>>();
            auto sink2 = std::make_shared<NullSink<String>>();
            auto sink3 = std::make_shared<NullSink<String>>();

            std::latch start_latch(num_threads + 1);
            std::vector<std::thread> threads;
            std::atomic<bool> should_stop{false};
            std::atomic<int> operations_completed{0};

            // Launch threads that will trigger update_propagated_sinks and set_parent concurrently
            for (int i = 0; i < num_threads; ++i) {
                threads.emplace_back([&, thread_id = i]() {
                    start_latch.arrive_and_wait();

                    int local_ops = 0;
                    while (!should_stop.load(std::memory_order_relaxed) && local_ops < 15) {
                        if (thread_id % 3 == 0) {
                            // Add/remove sinks from root (triggers update_propagated_sinks down
                            // the hierarchy) This exercises parent→child locking in
                            // update_propagated_sinks
                            root_logger->add_sink(sink1);
                            root_logger->remove_sink(sink1);
                            ++local_ops;
                        } else if (thread_id % 3 == 1) {
                            // Change parent relationships (triggers address-ordered locking)
                            // This exercises address-based lock ordering in set_parent
                            child_logger->set_parent(root_logger); // Skip intermediate parent
                            child_logger->set_parent(parent_logger); // Back to original
                            ++local_ops;
                        } else {
                            // Add/remove sinks from parent while changing child's parent
                            // This creates complex lock interaction scenarios
                            parent_logger->add_sink(sink2);
                            child_logger->set_parent(nullptr);
                            parent_logger->remove_sink(sink2);
                            child_logger->set_parent(parent_logger);
                            ++local_ops;
                        }

                        // Encourage context switching
                        std::this_thread::yield();
                    }
                    operations_completed.fetch_add(local_ops, std::memory_order_relaxed);
                });
            }

            // Start all threads simultaneously
            start_latch.arrive_and_wait();

            // Let them run for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            should_stop.store(true, std::memory_order_relaxed);

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Test passes if we didn't deadlock and some operations completed
            expect(operations_completed.load(), greater(0));
        }
    });

    // Test complex parent relationship changes (lock ordering verification)
    _.test("parent_relationships", []() {
        constexpr int num_iterations = 20;
        constexpr int num_threads = 10;

        for (int iter = 0; iter < num_iterations; ++iter) {
            // Create multiple loggers that will be rearranged
            std::vector<std::shared_ptr<Logger<String, Char, MultiThreadedPolicy>>> loggers;
            for (int i = 0; i < 6; ++i) {
                loggers.push_back(
                    std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                        from_utf8<Char>("logger_" + std::to_string(i))));
            }

            std::latch start_latch(num_threads + 1);
            std::vector<std::thread> threads;
            std::atomic<bool> should_stop{false};
            std::atomic<int> relationship_changes{0};

            // Launch threads that will create complex parent-child relationships
            for (int i = 0; i < num_threads; ++i) {
                threads.emplace_back([&, thread_id = i]() {
                    start_latch.arrive_and_wait();

                    int local_changes = 0;
                    while (!should_stop.load(std::memory_order_relaxed) && local_changes < 10) {
                        // Create various parent-child relationships that exercise
                        // different address orderings (both directions)
                        const int parent_idx = thread_id % loggers.size();
                        const int child_idx = (thread_id + 1) % loggers.size();

                        if (parent_idx != child_idx) {
                            // This will exercise address-based lock ordering in both directions
                            // depending on memory layout
                            loggers[child_idx]->set_parent(loggers[parent_idx]);
                            loggers[child_idx]->set_parent(nullptr); // Clear
                            ++local_changes;
                        }

                        // Create chain relationships
                        if (local_changes % 3 == 0) {
                            const int logger1 = thread_id % loggers.size();
                            const int logger2 = (thread_id + 2) % loggers.size();
                            const int logger3 = (thread_id + 3) % loggers.size();

                            if (logger1 != logger2 && logger2 != logger3 && logger1 != logger3) {
                                // Create chain: logger1 -> logger2 -> logger3
                                loggers[logger2]->set_parent(loggers[logger1]);
                                loggers[logger3]->set_parent(loggers[logger2]);
                                // Break chain
                                loggers[logger3]->set_parent(nullptr);
                                loggers[logger2]->set_parent(nullptr);
                                ++local_changes;
                            }
                        }

                        // Encourage interleaving
                        std::this_thread::yield();
                    }
                    relationship_changes.fetch_add(local_changes, std::memory_order_relaxed);
                });
            }

            // Start all threads simultaneously
            start_latch.arrive_and_wait();

            // Let them run for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            should_stop.store(true, std::memory_order_relaxed);

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Test passes if we didn't deadlock and relationships were changed
            expect(relationship_changes.load(), greater(0));
        }
    });

    // Test sink propagation correctness under concurrent modifications
    _.test("sink_propagation", []() {
        constexpr int num_iterations = 20;
        constexpr int num_threads = 8;

        for (int iter = 0; iter < num_iterations; ++iter) {
            // Create hierarchy: root -> intermediate -> leaf
            auto root = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                from_utf8<Char>("root"));
            auto intermediate = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                root, from_utf8<Char>("intermediate"));
            auto leaf = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                intermediate, from_utf8<Char>("leaf"));

            // Create test sinks
            std::vector<std::shared_ptr<NullSink<String>>> sinks;
            for (int i = 0; i < 5; ++i) {
                sinks.push_back(std::make_shared<NullSink<String>>());
            }

            std::latch start_latch(num_threads + 1);
            std::vector<std::thread> threads;
            std::atomic<bool> should_stop{false};
            std::atomic<int> operations{0};

            // Launch threads that will modify sinks and hierarchy concurrently
            for (int i = 0; i < num_threads; ++i) {
                threads.emplace_back([&, thread_id = i]() {
                    start_latch.arrive_and_wait();

                    int local_ops = 0;
                    while (!should_stop.load(std::memory_order_relaxed) && local_ops < 12) {
                        if (thread_id % 4 == 0) {
                            // Add/remove sinks from root (should propagate to children)
                            const auto sink = sinks[local_ops % sinks.size()];
                            root->add_sink(sink);
                            root->remove_sink(sink);
                            ++local_ops;
                        } else if (thread_id % 4 == 1) {
                            // Change propagation settings while sinks are being modified
                            intermediate->set_propagate(false);
                            intermediate->set_propagate(true);
                            ++local_ops;
                        } else if (thread_id % 4 == 2) {
                            // Change parent relationships while sinks are active
                            leaf->set_parent(root); // Skip intermediate
                            leaf->set_parent(intermediate); // Back to original
                            ++local_ops;
                        } else {
                            // Add/remove sinks from intermediate level
                            const auto sink = sinks[(local_ops + 2) % sinks.size()];
                            intermediate->add_sink(sink);
                            intermediate->set_sink_enabled(sink, false);
                            intermediate->set_sink_enabled(sink, true);
                            intermediate->remove_sink(sink);
                            ++local_ops;
                        }

                        // Brief pause to encourage interleaving
                        std::this_thread::yield();
                    }
                    operations.fetch_add(local_ops, std::memory_order_relaxed);
                });
            }

            // Start all threads simultaneously
            start_latch.arrive_and_wait();

            // Let them run for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            should_stop.store(true, std::memory_order_relaxed);

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Test passes if we completed operations without hanging or crashing
            expect(operations.load(), greater(0));

            // Verify final state is consistent (no dangling references)
            // All loggers should be in a valid state
            expect(root->parent(), equal_to(nullptr));
            expect(intermediate->parent(), equal_to(root));
            expect(leaf->parent(), equal_to(intermediate));
        }
    });

    // Comprehensive stress test for all threading scenarios
    _.test("stress_test", []() {
        constexpr int num_iterations = 10;
        constexpr int num_threads = 12;
        constexpr int runtime_ms = 100;

        for (int iter = 0; iter < num_iterations; ++iter) {
            // Create a moderately complex hierarchy
            auto root = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                from_utf8<Char>("root"));

            std::vector<std::shared_ptr<Logger<String, Char, MultiThreadedPolicy>>> loggers;
            loggers.push_back(root);

            // Create some initial children
            for (int i = 0; i < 4; ++i) {
                auto logger = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                    root, from_utf8<Char>("child_" + std::to_string(i)));
                loggers.push_back(logger);
            }

            // Create sinks
            std::vector<std::shared_ptr<NullSink<String>>> sinks;
            for (int i = 0; i < 3; ++i) {
                sinks.push_back(std::make_shared<NullSink<String>>());
            }

            std::latch start_latch(num_threads + 1);
            std::vector<std::thread> threads;
            std::atomic<bool> should_stop{false};
            std::atomic<int> total_operations{0};

            // Launch threads with different responsibilities
            for (int i = 0; i < num_threads; ++i) {
                threads.emplace_back([&, thread_id = i]() {
                    start_latch.arrive_and_wait();

                    int local_ops = 0;
                    std::mt19937 rng(thread_id);

                    while (!should_stop.load(std::memory_order_relaxed)) {
                        const int operation = rng() % 7;

                        switch (operation) {
                        case 0: {
                            // Create new logger with random parent
                            const int parent_idx = rng() % loggers.size();
                            auto new_logger
                                = std::make_shared<Logger<String, Char, MultiThreadedPolicy>>(
                                    loggers[parent_idx],
                                    from_utf8<Char>(
                                        "dynamic_" + std::to_string(thread_id) + "_"
                                        + std::to_string(local_ops)));
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
                        }
                        ++local_ops;

                        // Brief yield to encourage interleaving
                        if (local_ops % 5 == 0) {
                            std::this_thread::yield();
                        }
                    }
                    total_operations.fetch_add(local_ops, std::memory_order_relaxed);
                });
            }

            // Start all threads simultaneously
            start_latch.arrive_and_wait();

            // Let them run for the specified time
            std::this_thread::sleep_for(std::chrono::milliseconds(runtime_ms));
            should_stop.store(true, std::memory_order_relaxed);

            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }

            // Test passes if we completed many operations without deadlock/crash
            expect(total_operations.load(), greater(100));
        }
    });
});

} // namespace
