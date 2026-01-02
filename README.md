# SlimLog

**SlimLog** is a lightweight, high-performance, and flexible logging library for C++20. Inspired by Python's `logging` module, it features a powerful hierarchical logger model, making it ideal for complex applications with modular architectures.

## Features

*   **Hierarchical Logging:** Supports parent/child logger relationships with message propagation, similar to Python's `logging` module.
*   **Type Agnostic:**
    *   **Char Type:** Fully supports `char`, `wchar_t`, `char8_t`, `char16_t`, and `char32_t` (compiler permitting).
    *   **String Type:** Can handle any string type (e.g., `QString`, `CString`) via `ConvertString` specialization.
*   **Configurable:** Buffer sizes for log messages and sinks can be tuned via template arguments.
*   **Flexible Formatting:**
    *   Built-in support for **[fmtlib](https://github.com/fmtlib/fmt)** (header-only or linked) for maximum performance.
    *   Fallback to C++20 `std::format` for a dependency-free experience.
    *   Optimized for contiguous buffers.
*   **Extensible Sinks:**
    *   `OStreamSink`: Log to `std::cout`, `std::cerr`, or any `std::ostream`.
    *   `FileSink`: Log to files.
    *   `CallbackSink`: Custom handling via lambdas/functions.
    *   `QMessageLoggerSink`: Integration with Qt's `QMessageLogger`.
    *   `NullSink`: For benchmarking or disabling output.
*   **Thread Safety:** Configurable threading policy (`SingleThreadedPolicy` or `MultiThreadedPolicy`).
*   **Minimalistic:** Clean, modern C++20 codebase. Can be used as a header-only library or built as a static/shared library.

## Installation

### CMake

You can integrate SlimLog using CMake's `FetchContent`, by adding it as a subdirectory, or using `find_package` if installed.

**Using FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(
    slimlog
    GIT_REPOSITORY https://github.com/polter-rnd/slimlog.git
    GIT_TAG master
)
FetchContent_MakeAvailable(slimlog)

target_link_libraries(your_target PRIVATE slimlog::slimlog)
```

**Using find_package:**

```cmake
find_package(slimlog REQUIRED)
target_link_libraries(your_target PRIVATE slimlog::slimlog)
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `SLIMLOG_FMTLIB` | Use `fmtlib` for formatting (recommended for performance). | `ON` |
| `SLIMLOG_FMTLIB_HO` | Use `fmtlib` in header-only mode. | `OFF` |
| `SLIMLOG_TESTS` | Build unit tests. | `OFF` |
| `SLIMLOG_DOCS` | Build Doxygen documentation. | `OFF` |
| `SLIMLOG_COVERAGE` | Enable code coverage support (gcov, llvmcov). | `OFF` |
| `SLIMLOG_ANALYZERS` | Enable static analyzers (clang-tidy, cppcheck, iwyu). | `OFF` |
| `SLIMLOG_SANITIZERS` | Enable sanitizers (asan, lsan, msan, tsan, ubsan). | `OFF` |
| `SLIMLOG_FORMATTERS` | Enable code formatting targets (`format`, `formatcheck`). | `OFF` |

*   **Performance Note:** Using `SLIMLOG_FMTLIB=ON` is recommended as it includes optimizations for contiguous buffers that significantly improve performance compared to standard streams or unoptimized `std::format`.

## Usage

### Basic Logging

```cpp
#include <slimlog/logger.h>
#include <slimlog/sinks/ostream_sink.h>

int main() {
    // Create a logger with a console sink
    auto logger = SlimLog::create_logger(SlimLog::Level::Info);
    logger->add_sink<SlimLog::OStreamSink>(std::cout);

    logger->info("Hello, {}!", "World");
    logger->error("Something went wrong: error code {}", 404);

    return 0;
}
```

### Hierarchical Logging

SlimLog's killer feature is its hierarchy model. Child loggers can propagate messages to their parents, allowing for centralized configuration of sinks while maintaining granular control over logging levels.

```cpp
#include <slimlog/logger.h>
#include <slimlog/sinks/ostream_sink.h>

int main() {
    // Root logger setup
    auto root = SlimLog::create_logger("root", SlimLog::Level::Warning);
    root->add_sink<SlimLog::OStreamSink>(std::cerr); // Log warnings and errors to stderr

    // Child logger for a specific module
    auto network_logger = SlimLog::create_logger(root, "network", SlimLog::Level::Debug);
    
    // This message is propagated to 'root' but filtered out because root level is Warning
    // However, if we added a sink to network_logger, it would show up there.
    network_logger->info("Network initialized"); 

    // This propagates to root and is printed because it's an Error
    network_logger->error("Connection failed"); 

    return 0;
}
```

### Using Different Character Types

SlimLog is agnostic to the character type used.

```cpp
#include <slimlog/logger.h>
#include <slimlog/sinks/ostream_sink.h>

int main() {
    // Wide character logger
    auto wlogger = SlimLog::create_logger<wchar_t>(SlimLog::Level::Info);
    wlogger->add_sink<SlimLog::OStreamSink<wchar_t>>(std::wcout);

    wlogger->info(L"Unicode support: \u2713");
    
    return 0;
}
```

### Callback Sink

Useful for integrating with other systems or custom processing.

```cpp
#include <slimlog/logger.h>
#include <slimlog/sinks/callback_sink.h>

int main() {
    auto logger = SlimLog::create_logger();
    
    logger->add_sink<SlimLog::CallbackSink>([](SlimLog::Level level, const SlimLog::Location& loc, std::string_view msg) {
        // Custom handling logic
        std::cout << "[Custom] " << msg << "\n";
    });

    logger->info("Callback test");
    return 0;
}
```

## Sinks

Sinks are the destinations for log messages. SlimLog provides several built-in sinks, and you can easily create your own.

*   **`OStreamSink`**: Writes to standard output streams (`std::cout`, `std::cerr`) or file streams.
*   **`FileSink`**: Writes directly to a file.
*   **`CallbackSink`**: Delegates logging to a user-provided callback function.
*   **`QMessageLoggerSink`**: Forwards logs to Qt's logging system.
*   **`NullSink`**: Discards all messages (useful for testing).

Sinks are designed to be thread-safe by default. They manage their own synchronization, so you don't need to worry about race conditions when multiple loggers write to the same sink.

## Thread Safety

The `ThreadingPolicy` template parameter in the `Logger` class controls the thread safety of the **Logger instance itself**. This includes operations like:
*   Changing the log level.
*   Adding or removing sinks.
*   Modifying the logger hierarchy (adding/removing children).
*   Enabling/disabling propagation.

It **does not** affect the sinks, which handle their own thread safety.

By default, loggers use `SingleThreadedPolicy`. For multi-threaded applications where you might be modifying the logger configuration from multiple threads, specify the policy when defining the logger type:

```cpp
#include <slimlog/logger.h>
#include <slimlog/threading.h>

using MtLogger = SlimLog::Logger<char, SlimLog::MultiThreadedPolicy>;

int main() {
    auto logger = MtLogger::create();
    // Safe to modify logger from multiple threads
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
