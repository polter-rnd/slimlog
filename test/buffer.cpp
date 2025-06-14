// SlimLog
#include "slimlog/util/buffer.h"

#include "slimlog/util/unicode.h"

// Test helpers
#include "helpers/common.h"

#include <mettle.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <new>
#include <string>
#include <string_view>

#ifdef SLIMLOG_FMTLIB
#if __has_include(<fmt/base.h>)
#include <fmt/base.h>
#else
#include <fmt/core.h>
#endif
#else
// IWYU pragma: no_include <fmt/base.h>
// IWYU pragma: no_include <fmt/core.h>
#endif

// IWYU pragma: no_include <vector>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog::Util;

// Custom allocator that limits the maximum allocation size
template<typename T, std::size_t MaxSize = 1000>
struct LimitedAllocator {
    using value_type = T; // NOLINT(readability-identifier-naming)

    constexpr LimitedAllocator() noexcept = default;

    constexpr auto allocate(std::size_t size) -> T*
    {
        if (size > MaxSize) {
            throw std::bad_alloc{};
        }
        return std::allocator<T>{}.allocate(size);
    }

    constexpr void deallocate(T* data, std::size_t size) noexcept
    {
        std::allocator<T>{}.deallocate(data, size);
    }

    [[nodiscard]] constexpr auto max_size() const noexcept -> std::size_t
    {
        return MaxSize;
    }
};

// Constexpr test helpers - these will only compile if the functions are truly constexpr
template<typename Char>
constexpr auto test_constexpr_basic() -> bool
{
    using BufferType = MemoryBuffer<Char, 256>;
    BufferType buffer;
    buffer.push_back(Char{'A'});
    buffer.resize(5);
    buffer[1] = Char{'B'};
    auto* data = buffer.data();
    data[2] = Char{'C'};
    return buffer.size() == 5 && buffer[0] == Char{'A'} && buffer[1] == Char{'B'}
    && buffer[2] == Char{'C'} && buffer.capacity() == 256;
}

template<typename Char>
constexpr auto test_constexpr_move() -> bool
{
    using BufferType = MemoryBuffer<Char, 256>;
    BufferType original;
    original.push_back(Char{'M'});
    BufferType moved(std::move(original));
    return moved.size() == 1 && moved[0] == Char{'M'};
}

#if !defined(SLIMLOG_FMTLIB) || FMT_VERSION >= 110100
template<typename Char>
constexpr auto test_constexpr_clear() -> bool
{
    using BufferType = MemoryBuffer<Char, 256>;
    BufferType buffer;
    buffer.push_back(Char{'X'});
    buffer.clear();
    return buffer.size() == 0;
}
#else
template<typename Char>
constexpr auto test_constexpr_clear() -> bool
{
    return true; // clear method is not constexpr in fmtlib < 11.1
}
#endif

// Runtime test suite for MemoryBuffer
const suite<SLIMLOG_CHAR_TYPES> BufferTests("buffer", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;
    using StringView = std::basic_string_view<Char>;
    using BufferType = MemoryBuffer<Char, 256>;

    // Test constexpr capabilities (compile-time evaluation)
    static_assert(test_constexpr_basic<Char>());
    static_assert(test_constexpr_move<Char>());
    static_assert(test_constexpr_clear<Char>());

    // Test default construction
    _.test("default_construction", []() {
        BufferType buffer;
        expect(buffer.size(), equal_to(0U));
        expect(buffer.capacity(), greater(0U));
        expect(buffer.data(), is_not(nullptr));
    });

    // Test construction with initial capacity
    _.test("capacity_construction", []() {
        constexpr std::size_t InitialCapacity = 100;
        using CustomBufferType = MemoryBuffer<Char, InitialCapacity>;
        CustomBufferType buffer;
        expect(buffer.size(), equal_to(0U));
        expect(buffer.capacity(), greater_equal(InitialCapacity));
        expect(buffer.data(), is_not(nullptr));
    });

    // Test move operations with stack storage
    _.test("move_stack", []() {
        BufferType original;
        const auto test_string = from_utf8<Char>("Hello, World!");
        original.append(test_string);
        const auto original_size = original.size();

        // Test move constructor
        BufferType moved_construct(std::move(original));
        expect(moved_construct.size(), equal_to(original_size));
        expect(moved_construct.capacity(), greater(0U));
        expect(StringView(moved_construct.data(), moved_construct.size()), equal_to(test_string));

        // Test move assignment (reuse the moved buffer)
        BufferType moved_assign;
        moved_assign = std::move(moved_construct);
        expect(moved_assign.size(), equal_to(original_size));
        expect(StringView(moved_assign.data(), moved_assign.size()), equal_to(test_string));
    });

    // Test move operations with heap storage
    _.test("move_heap", []() {
        using SmallBufferType = MemoryBuffer<Char, 4>; // Very small stack
        SmallBufferType original;
        const auto large_string
            = from_utf8<Char>("This string exceeds 4 chars and forces heap allocation");
        original.append(large_string);

        // Verify heap allocation occurred
        expect(original.capacity(), greater(4U));
        auto* heap_data = original.data();

        // Test move constructor
        SmallBufferType moved_construct(std::move(original));
        expect(moved_construct.size(), equal_to(large_string.size()));
        expect(StringView(moved_construct.data(), moved_construct.size()), equal_to(large_string));
        expect(moved_construct.data(), equal_to(heap_data)); // Should transfer heap pointer

        SmallBufferType moved_assign;
        heap_data = moved_construct.data();
        moved_assign = std::move(moved_construct);
        expect(moved_assign.size(), equal_to(large_string.size()));
        expect(StringView(moved_assign.data(), moved_assign.size()), equal_to(large_string));
        expect(moved_assign.data(), equal_to(heap_data)); // Should transfer heap pointer
    });

    // Test append operations
    _.test("append", []() {
        BufferType buffer;

        // Append single character
        buffer.push_back(Char{'A'});
        expect(buffer.size(), equal_to(1U));
        expect(buffer[0], equal_to(Char{'A'}));

        // Append string
        buffer.append(from_utf8<Char>("BCD"));
        expect(buffer.size(), equal_to(4U));
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("ABCD")));
    });

    // Test clear operation
    _.test("clear", []() {
        BufferType buffer;
        const auto test_string = from_utf8<Char>("Hello, World!");
        buffer.append(test_string);

        expect(buffer.size(), greater(0U));
        buffer.clear();
        expect(buffer.size(), equal_to(0U));
        expect(buffer.capacity(), greater(0U)); // Capacity should remain
    });

    // Test reserve operation
    _.test("reserve", []() {
        BufferType buffer;
        const std::size_t new_capacity = 500;

        buffer.reserve(new_capacity);
        expect(buffer.capacity(), greater_equal(new_capacity));
        expect(buffer.size(), equal_to(0U));

        // Should not reallocate when appending within reserved capacity
        const auto* original_data = buffer.data();
        const auto test_string = from_utf8<Char>("Test");
        buffer.append(test_string);
        expect(buffer.data(), equal_to(original_data));
    });

    // Test resize operation
    _.test("resize", []() {
        BufferType buffer;

        // Resize to larger size
        buffer.resize(10);
        expect(buffer.size(), equal_to(10U));
        expect(buffer.capacity(), greater_equal(10U));

        // Fill with known value
        std::fill(buffer.begin(), buffer.end(), Char{'X'});
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("XXXXXXXXXX")));

        // Resize to smaller size
        buffer.resize(5);
        expect(buffer.size(), equal_to(5U));
        expect(StringView(buffer.data(), buffer.size()), equal_to(from_utf8<Char>("XXXXX")));
    });

    // Test iterator operations
    _.test("iterators", []() {
        BufferType buffer;
        const auto test_string = from_utf8<Char>("Hello");
        buffer.append(test_string);

        // Test iterators
        expect(
            std::distance(buffer.begin(), buffer.end()),
            equal_to(static_cast<std::ptrdiff_t>(buffer.size())));

        // Test iterator content
        const String reconstructed(buffer.begin(), buffer.end());
        expect(reconstructed, equal_to(test_string));

        // Test modification through iterators
        if (buffer.size() > 0) {
            *buffer.begin() = Char{'h'};
            expect(buffer[0], equal_to(Char{'h'}));
        }
    });

    // Test reallocation behavior
    _.test("reallocation", []() {
        using SmallBufferType = MemoryBuffer<Char, 2>; // Start with small capacity
        SmallBufferType buffer;
        const auto* original_data = buffer.data();

        // Fill beyond initial capacity to force reallocation
        const auto long_string
            = from_utf8<Char>("This is a very long string that should exceed the initial capacity");
        buffer.append(long_string);

        // Data pointer should change after reallocation
        expect(buffer.data(), not_equal_to(original_data));
        expect(buffer.size(), equal_to(long_string.size()));
        expect(StringView(buffer.data(), buffer.size()), equal_to(long_string));
        expect(buffer.capacity(), greater_equal(buffer.size()));
    });

    // Test large data handling
    _.test("large_data", []() {
        BufferType buffer;

        // Create large string
        const std::size_t large_size = 100000;
        String large_string;
        large_string.reserve(large_size);
        for (std::size_t i = 0; i < large_size; ++i) {
            large_string.push_back(static_cast<Char>('A' + (i % 26)));
        }

        buffer.append(large_string);
        expect(buffer.size(), equal_to(large_size));
        expect(StringView(buffer.data(), buffer.size()), equal_to(large_string));
    });

    // Test unicode handling
    _.test("unicode", []() {
        BufferType buffer;

        // Test with various unicode strings
        for (const auto& str : unicode_strings<Char>()) {
            buffer.clear();
            buffer.append(str);
            expect(buffer.size(), equal_to(str.size()));
            expect(StringView(buffer.data(), buffer.size()), equal_to(str));
        }
    });

    // Test edge cases
    _.test("edge_cases", []() {
        BufferType buffer;

        // Test empty append operations
        const auto empty_string = String{};
        buffer.append(empty_string);
        expect(buffer.size(), equal_to(0U));

        const auto empty_view = StringView{};
        buffer.append(empty_view);
        expect(buffer.size(), equal_to(0U));

        // Test with zero-size operations
        buffer.resize(0);
        buffer.reserve(0);

        expect(buffer.size(), equal_to(0U));
        expect(buffer.capacity(), greater(0U));
    });

    // Test allocator large limits with custom allocator
    _.test("allocator_large", []() {
        // Test with large allocator for TRUE branch
        using LargeBufferType = MemoryBuffer<Char, 10, LimitedAllocator<Char, 1000>>;
        LargeBufferType buffer;

        const auto max_size = buffer.get_allocator().max_size();
        expect(max_size, equal_to(1000U));

        // Branch 1: Test when size > new_capacity (new_capacity = size)
        const std::size_t large_jump = max_size - 50;
        buffer.resize(large_jump);
        expect(buffer.size(), equal_to(large_jump));
        expect(buffer.capacity(), greater_equal(large_jump));

        // Branch 2a: Test when new_capacity > max_size is TRUE
        buffer.clear();
        const std::size_t near_max = max_size - 5;
        buffer.resize(near_max);
        const std::size_t trigger_size = near_max + 1;
        buffer.resize(trigger_size);
        expect(buffer.size(), equal_to(trigger_size));
        expect(buffer.capacity(), less_equal(max_size));

        // Test exception behavior
        expect([&buffer, max_size]() { buffer.resize(max_size + 1); }, thrown<std::bad_alloc>());
        expect(buffer.size(), less_equal(max_size));
    });

    // Test allocator small limits with custom allocator
    _.test("allocator_small", []() {
        // Branch 2b: Test when new_capacity > max_size is FALSE
        // Use small allocator to ensure false branch
        using SmallBufferType = MemoryBuffer<Char, 5, LimitedAllocator<Char, 25>>;
        SmallBufferType small_buffer;

        // Buffer starts with stack capacity = 5
        // Resize to 3, which fits in stack (capacity remains 5)
        small_buffer.resize(3);

        // Growth formula: 5 + (5 / 2) = 7
        // Since 7 <= 25 (max_size), this will hit the FALSE branch

        // Force reallocation by exceeding stack capacity (5)
        small_buffer.resize(6); // 6 > 5, triggers growth to capacity 7

        expect(small_buffer.size(), equal_to(6U));
        expect(small_buffer.capacity(), equal_to(7U)); // Growth formula result
        expect(small_buffer.capacity(), less(25U)); // Proves false branch was taken
    });
});

} // namespace
