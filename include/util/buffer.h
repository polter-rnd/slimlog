#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>

#ifdef ENABLE_FMTLIB
template<typename T>
using Buffer = fmt::detail::buffer<T>;
#else
template<typename T>
class Buffer {
private:
    T* m_ptr;
    size_t m_size;
    size_t m_capacity;

    using GrowCallback = void (*)(Buffer& buf, size_t capacity);
    GrowCallback m_grow;

protected:
    constexpr Buffer(GrowCallback grow, size_t sz) noexcept
        : m_size(sz)
        , m_capacity(sz)
        , m_grow(grow)
    {
    }

    constexpr Buffer(
        GrowCallback grow, T* ptr = nullptr, size_t size = 0, size_t capacity = 0) noexcept
        : m_ptr(ptr)
        , m_size(size)
        , m_capacity(capacity)
        , m_grow(grow)
    {
    }

    constexpr ~Buffer() = default;
    Buffer(Buffer&&) = default;

    /// Sets the buffer data and capacity.
    constexpr void set(T* buf_data, size_t buf_capacity) noexcept
    {
        m_ptr = buf_data;
        m_capacity = buf_capacity;
    }

public:
    using value_type = T;
    using const_reference = const T&;

    Buffer(const Buffer&) = delete;
    void operator=(const Buffer&) = delete;

    auto begin() noexcept -> T*
    {
        return m_ptr;
    }
    auto end() noexcept -> T*
    {
        return m_ptr + m_size;
    }

    auto begin() const noexcept -> const T*
    {
        return m_ptr;
    }
    auto end() const noexcept -> const T*
    {
        return m_ptr + m_size;
    }

    /// Returns the size of this buffer.
    constexpr auto size() const noexcept -> size_t
    {
        return m_size;
    }

    /// Returns the capacity of this buffer.
    constexpr auto capacity() const noexcept -> size_t
    {
        return m_capacity;
    }

    /// Returns a pointer to the buffer data (not null-terminated).
    constexpr auto data() noexcept -> T*
    {
        return m_ptr;
    }
    constexpr auto data() const noexcept -> const T*
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
    constexpr void try_resize(size_t count)
    {
        try_reserve(count);
        m_size = count <= m_capacity ? count : m_capacity;
    }

    // Tries increasing the buffer capacity to `new_capacity`. It can increase the
    // capacity by a smaller amount than requested but guarantees there is space
    // for at least one additional element either by increasing the capacity or by
    // flushing the buffer if it is full.
    constexpr void try_reserve(size_t new_capacity)
    {
        if (new_capacity > m_capacity) {
            m_grow(*this, new_capacity);
        }
    }

    constexpr void push_back(const T& value)
    {
        try_reserve(m_size + 1);
        m_ptr[m_size++] = value;
    }

    /// Appends data to the end of the buffer.
    template<typename U>
    void append(const U* begin, const U* end)
    {
        while (begin != end) {
            auto count = static_cast<std::make_unsigned_t<decltype(end - begin)>>(end - begin);
            try_reserve(m_size + count);
            auto free_cap = m_capacity - m_size;
            if (free_cap < count)
                count = free_cap;
            if (std::is_same<T, U>::value) {
                // memcpy(m_ptr + m_size, begin, count * sizeof(T));
                std::uninitialized_copy_n(begin, count, m_ptr + m_size);
            } else {
                T* out = m_ptr + m_size;
                for (size_t i = 0; i < count; ++i)
                    out[i] = begin[i];
            }
            // A loop is faster than memcpy on small sizes.
            /*T* out = m_ptr + m_size;
            for (size_t i = 0; i < count; ++i) {
                out[i] = begin[i];
            }*/

            m_size += count;
            begin += count;
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
};
#endif

template<typename T, size_t Size = 1024, typename Allocator = std::allocator<T>>
class MemoryBuffer : public Buffer<T> {
private:
    using OnGrowCallback = std::function<void(size_t, size_t)>;

    T m_store[Size];
    Allocator m_allocator;
    OnGrowCallback m_on_grow;

    constexpr void deallocate()
    {
        T* data = this->data();
        if (data != m_store) {
            m_allocator.deallocate(data, this->capacity());
        }
    }

protected:
#if defined(ENABLE_FMTLIB) && FMT_VERSION < 110000
    constexpr void grow(size_t size) override
    {
        auto& self = *this;
#else
    static constexpr void grow(Buffer<T>& buf, size_t size)
    {
        auto& self = static_cast<MemoryBuffer&>(buf);
#endif
        const size_t max_size = std::allocator_traits<Allocator>::max_size(self.m_allocator);
        const size_t old_capacity = self.capacity();
        size_t new_capacity = old_capacity + old_capacity / 2;
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
        if (old_data != self.m_store) {
            self.m_allocator.deallocate(old_data, old_capacity);
        }
        if (self.m_on_grow) {
            self.m_on_grow(old_capacity, new_capacity);
        }
    }

public:
    using value_type = T;
    using const_reference = const T&;

    constexpr explicit MemoryBuffer(const Allocator& allocator = Allocator())
#if defined(ENABLE_FMTLIB) && FMT_VERSION < 110000
        : m_allocator(alloc)
#else
        : Buffer<T>(grow)
        , m_allocator(allocator)
#endif
    {
        this->set(m_store, Size);
        if (std::is_constant_evaluated()) {
            std::fill_n(m_store, Size, T());
        }
    }

    /*constexpr*/ ~MemoryBuffer()
    {
        deallocate();
    }

    void on_grow(const OnGrowCallback& callback)
    {
        m_on_grow = callback;
    }

private:
    // Move data from other to this buffer.
    constexpr void move(MemoryBuffer& other)
    {
        m_allocator = std::move(other.m_allocator);
        T* data = other.data();
        const size_t size = other.size();
        const size_t capacity = other.capacity();
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

public:
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

    // Returns a copy of the allocator associated with this buffer.
    [[nodiscard]] auto get_allocator() const -> Allocator
    {
        return m_allocator;
    }

    /// Resizes the buffer to contain `count` elements. If T is a POD type new
    /// elements may not be initialized.
    constexpr void resize(size_t count)
    {
        this->try_resize(count);
    }

    /// Increases the buffer capacity to `new_capacity`.
    void reserve(size_t new_capacity)
    {
        this->try_reserve(new_capacity);
    }

    template<typename ContiguousRange>
    void append(const ContiguousRange& range)
    {
        Buffer<T>::append(range.data(), range.data() + range.size());
    }
};