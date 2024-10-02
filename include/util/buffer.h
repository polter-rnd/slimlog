#pragma once

#include <algorithm> // IWYU pragma: keep
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#ifdef ENABLE_FMTLIB
template<typename T>
using Buffer = fmt::detail::buffer<T>;
#else
template<typename T>
class Buffer {
public:
    using value_type = T; // NOLINT(readability-identifier-naming)
    using const_reference = const T&; // NOLINT(readability-identifier-naming)

    Buffer(const Buffer&) = delete;
    void operator=(const Buffer&) = delete;

    auto begin() noexcept -> T*
    {
        return m_ptr;
    }
    auto end() noexcept -> T*
    {
        return std::next(m_ptr, m_size);
    }

    [[nodiscard]] auto begin() const noexcept -> const T*
    {
        return m_ptr;
    }
    [[nodiscard]] auto end() const noexcept -> const T*
    {
        return m_ptr + m_size;
    }

    /// Returns the size of this buffer.
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return m_size;
    }

    /// Returns the capacity of this buffer.
    [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t
    {
        return m_capacity;
    }

    /// Returns a pointer to the buffer data (not null-terminated).
    [[nodiscard]] constexpr auto data() noexcept -> T*
    {
        return m_ptr;
    }
    [[nodiscard]] constexpr auto data() const noexcept -> const T*
    {
        return m_ptr;
    }

    /// Clears this buffer.
    void clear()
    {
        m_size = 0;
    }

    // Tries resizing the buffer to contain `count` elements. If T is a POD type
    // the new elements may not be initialized.
    constexpr void try_resize(std::size_t count)
    {
        try_reserve(count);
        m_size = count <= m_capacity ? count : m_capacity;
    }

    // Tries increasing the buffer capacity to `new_capacity`. It can increase the
    // capacity by a smaller amount than requested but guarantees there is space
    // for at least one additional element either by increasing the capacity or by
    // flushing the buffer if it is full.
    constexpr void try_reserve(std::size_t new_capacity)
    {
        if (new_capacity > m_capacity) {
            m_grow(*this, new_capacity);
        }
    }

    constexpr void push_back(const T& value)
    {
        try_reserve(m_size + 1);
        *std::next(m_ptr, m_size++) = value;
    }

    /// Appends data to the end of the buffer.
    template<typename U>
    void append(const U* begin, const U* end)
    {
        while (begin != end) {
            auto count = static_cast<std::make_unsigned_t<decltype(end - begin)>>(end - begin);
            try_reserve(m_size + count);

            auto free_cap = m_capacity - m_size;
            if (free_cap < count) {
                count = free_cap;
            }
            if (std::is_same<T, U>::value) {
                std::uninitialized_copy_n(begin, count, std::next(m_ptr, m_size));
            } else {
                T* out = std::next(m_ptr, m_size);
                for (std::size_t i = 0; i < count; ++i) {
                    *std::next(out, i) = *std::next(begin, i);
                }
            }

            m_size += count;
            std::advance(begin, count);
        }
    }

    template<typename Idx>
    constexpr auto operator[](Idx index) -> T&
    {
        return m_ptr[index];
    }
    template<typename Idx>
    constexpr auto operator[](Idx index) const -> const T&
    {
        return m_ptr[index];
    }

protected:
    using GrowCallback = void (*)(Buffer& buf, std::size_t capacity);

    constexpr Buffer(GrowCallback grow, std::size_t size) noexcept
        : m_ptr(nullptr)
        , m_size(size)
        , m_capacity(size)
        , m_grow(grow)
    {
    }

    explicit constexpr Buffer(
        GrowCallback grow,
        T* ptr = nullptr,
        std::size_t size = 0,
        std::size_t capacity = 0) noexcept
        : m_ptr(ptr)
        , m_size(size)
        , m_capacity(capacity)
        , m_grow(grow)
    {
    }

    constexpr ~Buffer() = default;
    Buffer(Buffer&&) = default;
    auto operator=(Buffer&&) -> Buffer& = default;

    /// Sets the buffer data and capacity.
    constexpr void set(T* buf_data, std::size_t buf_capacity) noexcept
    {
        m_ptr = buf_data;
        m_capacity = buf_capacity;
    }

private:
    T* m_ptr;
    std::size_t m_size;
    std::size_t m_capacity;
    GrowCallback m_grow;
};
#endif

template<typename T, std::size_t Size, typename Allocator = std::allocator<T>>
class MemoryBuffer : public Buffer<T> {
public:
    using value_type = T; // NOLINT(readability-identifier-naming)
    using const_reference = const T&; // NOLINT(readability-identifier-naming)

    using OnGrowCallback = void (*)(const T*, std::size_t, void*);

    constexpr explicit MemoryBuffer(const Allocator& allocator = Allocator())
#if defined(ENABLE_FMTLIB) && FMT_VERSION < 110000
        : m_allocator(alloc)
#else
        : Buffer<T>(grow)
        , m_allocator(allocator)
#endif
    {
        this->set(static_cast<T*>(m_store), Size);
        if (std::is_constant_evaluated()) {
            std::fill_n(static_cast<T*>(m_store), Size, T());
        }
    }

    constexpr ~MemoryBuffer()
    {
        deallocate();
    }

    /// Constructs a `MemoryBuffer` object moving the content of the other
    /// object to it.
    constexpr MemoryBuffer(MemoryBuffer&& other) noexcept
        : Buffer<T>(grow)
    {
        move(other);
    }

    /// Moves the content of the other `MemoryBuffer` object to this one.
    auto operator=(MemoryBuffer&& other) noexcept -> MemoryBuffer&
    {
        deallocate();
        move(other);
        return *this;
    }

    constexpr MemoryBuffer(const MemoryBuffer& other) = delete;
    auto operator=(const MemoryBuffer& other) = delete;

    // Returns a copy of the allocator associated with this buffer.
    [[nodiscard]] auto get_allocator() const -> Allocator
    {
        return m_allocator;
    }

    /// Resizes the buffer to contain `count` elements. If T is a POD type new
    /// elements may not be initialized.
    constexpr void resize(std::size_t count)
    {
        this->try_resize(count);
    }

    /// Increases the buffer capacity to `new_capacity`.
    void reserve(std::size_t new_capacity)
    {
        this->try_reserve(new_capacity);
    }

    template<typename ContiguousRange>
    void append(const ContiguousRange& range)
    {
        Buffer<T>::append(range.data(), range.data() + range.size());
    }

    void on_grow(const OnGrowCallback& callback, void* userdata = nullptr)
    {
        m_on_grow = callback;
        m_on_grow_userdata = userdata;
    }

protected:
#if defined(ENABLE_FMTLIB) && FMT_VERSION < 110000
    constexpr void grow(std::size_t size) override
    {
        auto& self = *this;
#else
    static constexpr void grow(Buffer<T>& buf, std::size_t size)
    {
        auto& self = static_cast<MemoryBuffer&>(buf);
#endif
        const std::size_t max_size = std::allocator_traits<Allocator>::max_size(self.m_allocator);
        const std::size_t old_capacity = self.capacity();
        std::size_t new_capacity = old_capacity + old_capacity / 2;
        if (size > new_capacity) {
            new_capacity = size;
        } else if (new_capacity > max_size) {
            new_capacity = size > max_size ? size : max_size;
        }
        T* old_data = self.data();
        T* new_data = self.m_allocator.allocate(new_capacity);
        // std::cout << "GROW TO " << new_capacity << '\n';
        //  The following code doesn't throw, so the raw pointer above doesn't leak.
        //  memcpy(new_data, old_data, buf.size() * sizeof(T));
        std::uninitialized_copy_n(old_data, self.size(), new_data);
        self.set(new_data, new_capacity);
        // deallocate must not throw according to the standard, but even if it does,
        // the buffer already uses the new storage and will deallocate it in
        // destructor.
        if (old_data != static_cast<T*>(self.m_store)) {
            self.m_allocator.deallocate(old_data, old_capacity);
        }
        if (self.m_on_grow) {
            self.m_on_grow(new_data, new_capacity, self.m_on_grow_userdata);
        }
    }

private:
    constexpr void deallocate()
    {
        T* data = this->data();
        if (data != static_cast<T*>(m_store)) {
            m_allocator.deallocate(data, this->capacity());
        }
    }

    // Move data from other to this buffer.
    constexpr void move(MemoryBuffer& other)
    {
        m_allocator = std::move(other.m_allocator);
        T* data = other.data();
        const std::size_t size = other.size();
        const std::size_t capacity = other.capacity();
        if (data == other.m_store) {
            this->set(m_store, capacity);
            // m_store.append(other.m_store, other.m_store + size);
            std::uninitialized_copy_n(other.m_store, size, m_store);
        } else {
            this->set(data, capacity);
            // Set pointer to the inline array so that delete is not called
            // when deallocating.
            other.set(other.m_store, 0);
            other.clear();
        }
        this->resize(size);
    }

    T m_store[Size]; // NOLINT(*-avoid-c-arrays)
    Allocator m_allocator;
    OnGrowCallback m_on_grow = nullptr;
    void* m_on_grow_userdata = nullptr;
};
