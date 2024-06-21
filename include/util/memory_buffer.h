#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

template<typename T, size_t SIZE = 1024, typename Allocator = std::allocator<T>>
class MemoryBuffer {
private:
    T* ptr_;
    size_t size_;
    size_t capacity_;
    T store_[SIZE];
    Allocator alloc_;

    using grow_fun = void (*)(MemoryBuffer& buf, size_t capacity);
    grow_fun grow_;

    constexpr void deallocate()
    {
        T* data = this->data();
        if (data != store_)
            alloc_.deallocate(data, this->capacity());
    }

    static constexpr void grow(MemoryBuffer<T, SIZE, Allocator>& buf, size_t size)
    {
        auto& self = static_cast<MemoryBuffer&>(buf);
        const size_t max_size = std::allocator_traits<Allocator>::max_size(self.alloc_);
        size_t old_capacity = buf.capacity();
        size_t new_capacity = old_capacity + old_capacity / 2;
        if (size > new_capacity)
            new_capacity = size;
        else if (new_capacity > max_size)
            new_capacity = size > max_size ? size : max_size;
        T* old_data = buf.data();
        T* new_data = self.alloc_.allocate(new_capacity);
        // std::cout << "GROW TO " << new_capacity << '\n';
        // The following code doesn't throw, so the raw pointer above doesn't leak.
        // memcpy(new_data, old_data, buf.size() * sizeof(T));
        std::copy_n(old_data, buf.size(), new_data);
        self.set(new_data, new_capacity);
        // deallocate must not throw according to the standard, but even if it does,
        // the buffer already uses the new storage and will deallocate it in
        // destructor.
        if (old_data != self.store_)
            self.alloc_.deallocate(old_data, old_capacity);
    }

protected:
    /// Sets the buffer data and capacity.
    constexpr void set(T* buf_data, size_t buf_capacity) noexcept
    {
        ptr_ = buf_data;
        capacity_ = buf_capacity;
    }

    // Move data from other to this buffer.
    constexpr void move(MemoryBuffer& other)
    {
        alloc_ = std::move(other.alloc_);
        T* data = other.data();
        size_t size = other.size(), capacity = other.capacity();
        if (data == other.store_) {
            this->set(store_, capacity);
            store_.append(other.store_, other.store_ + size);
        } else {
            this->set(data, capacity);
            // Set pointer to the inline array so that delete is not called
            // when deallocating.
            other.set(other.store_, 0);
            other.clear();
        }
        this->resize(size);
    }

public:
    using value_type = T;
    using const_reference = const T&;

    void operator=(const MemoryBuffer&) = delete;
    // make another constructor to point to existing buffer. then use it for passing to
    // pattern->apply()
    constexpr explicit MemoryBuffer(const Allocator& alloc = Allocator())
        : ptr_(nullptr)
        , size_(0)
        , capacity_(0)
        , alloc_(alloc)
        , grow_(grow)
    {
        this->set(store_, SIZE);
        if (std::is_constant_evaluated())
            std::fill_n(store_, SIZE, T());
    }

    constexpr MemoryBuffer(T* buf, size_t size) noexcept
        : ptr_(buf)
        , size_(0)
        , capacity_(size)
        , grow_(grow)
    {
    }

    constexpr ~MemoryBuffer()
    {
        deallocate();
    }

    /// Constructs a `MemoryBuffer` object moving the content of the other
    /// object to it.
    constexpr MemoryBuffer(MemoryBuffer&& other) noexcept
        : ptr_(nullptr)
        , size_(0)
        , capacity_(0)
        , grow_(grow)
    {
        move(other);
    }

    // Returns a copy of the allocator associated with this buffer.
    auto get_allocator() const -> Allocator
    {
        return alloc_;
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
        append(range.data(), range.data() + range.size());
    }

    /// Moves the content of the other `MemoryBuffer` object to this one.
    auto operator=(MemoryBuffer&& other) noexcept -> MemoryBuffer&
    {
        deallocate();
        move(other);
        return *this;
    }

    auto begin() noexcept -> T*
    {
        return ptr_;
    }
    auto end() noexcept -> T*
    {
        return ptr_ + size_;
    }

    auto begin() const noexcept -> const T*
    {
        return ptr_;
    }
    auto end() const noexcept -> const T*
    {
        return ptr_ + size_;
    }

    /// Returns the size of this buffer.
    constexpr auto size() const noexcept -> size_t
    {
        return size_;
    }

    /// Returns the capacity of this buffer.
    constexpr auto capacity() const noexcept -> size_t
    {
        return capacity_;
    }

    /// Returns a pointer to the buffer data (not null-terminated).
    constexpr auto data() noexcept -> T*
    {
        return ptr_;
    }
    constexpr auto data() const noexcept -> const T*
    {
        return ptr_;
    }

    /// Clears this buffer.
    void clear()
    {
        size_ = 0;
    }

    // Tries resizing the buffer to contain `count` elements. If T is a POD type
    // the new elements may not be initialized.
    constexpr void try_resize(size_t count)
    {
        try_reserve(count);
        size_ = count <= capacity_ ? count : capacity_;
    }

    // Tries increasing the buffer capacity to `new_capacity`. It can increase the
    // capacity by a smaller amount than requested but guarantees there is space
    // for at least one additional element either by increasing the capacity or by
    // flushing the buffer if it is full.
    constexpr void try_reserve(size_t new_capacity)
    {
        if (new_capacity > capacity_)
            grow_(*this, new_capacity);
    }

    constexpr void push_back(const T& value)
    {
        try_reserve(size_ + 1);
        ptr_[size_++] = value;
    }

    /// Appends data to the end of the buffer.
    template<typename U>
    void append(const U* begin, const U* end)
    {
        while (begin != end) {
            size_t count = end - begin;
            try_reserve(size_ + count);
            auto free_cap = capacity_ - size_;
            if (free_cap < count)
                count = free_cap;
            if (std::is_same<T, U>::value) {
                // memcpy(ptr_ + size_, begin, count * sizeof(T));
                std::copy_n(begin, count, ptr_ + size_);
            } else {
                T* out = ptr_ + size_;
                for (size_t i = 0; i < count; ++i)
                    out[i] = begin[i];
            }

            size_ += count;
            begin += count;
        }
    }

    template<typename Idx>
    constexpr auto operator[](Idx index) -> T&
    {
        return ptr_[index];
    }
    template<typename Idx>
    constexpr auto operator[](Idx index) const -> const T&
    {
        return ptr_[index];
    }
};