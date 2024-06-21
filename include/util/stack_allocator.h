
#include <functional>
#include <memory>

template<class T, std::size_t N, class Allocator = std::allocator<T>>
class StackAllocator {
public:
    using value_type = T;

    template<class U>
    struct rebind {
        using other = StackAllocator<U, N, Allocator>;
    };

    StackAllocator(const StackAllocator&) noexcept = default;
    StackAllocator& operator=(const StackAllocator&) = delete;

    explicit StackAllocator(const Allocator& alloc = Allocator()) noexcept
        : m_allocator(alloc)
        , m_begin(nullptr)
        , m_end(nullptr)
        , m_stack_pointer(nullptr)
    {
    }

    explicit StackAllocator(T* buffer, const Allocator& alloc = Allocator()) noexcept
        : m_allocator(alloc)
        , m_begin(buffer)
        , m_end(buffer + N)
        , m_stack_pointer(buffer)
    {
    }

    template<class U>
    StackAllocator(const StackAllocator<U, N, Allocator>& other) noexcept
        : m_allocator(other.m_allocator)
        , m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_stack_pointer(other.m_stack_pointer)
    {
    }

    constexpr static size_t capacity()
    {
        return N;
    }

    [[nodiscard]] constexpr T* allocate(size_t n)
    {
        if (std::cmp_less_equal(n, std::distance(m_stack_pointer, m_end))) {
            T* result = m_stack_pointer;
            m_stack_pointer += n;
            return result;
        }

        return m_allocator.allocate(n);
    }

    void deallocate(T* p, size_t n)
    {
        if (!(std::less<const T*>()(p, m_begin)) && (std::less<const T*>()(p, m_end))) {
            m_stack_pointer -= n;
        } else
            m_allocator.deallocate(p, n);
    }

    // Buffer pointer declaration
    T* buffer() const noexcept
    {
        return m_begin;
    }

private:
    Allocator m_allocator;
    T *m_begin, *m_end, *m_stack_pointer;
};

template<class T1, std::size_t N, class Allocator, class T2>
bool operator==(
    const StackAllocator<T1, N, Allocator>& lhs,
    const StackAllocator<T2, N, Allocator>& rhs) noexcept
{
    return lhs.buffer() == rhs.buffer();
}
// class that will return
template<class T1, std::size_t N, class Allocator, class T2>
bool operator!=(
    const StackAllocator<T1, N, Allocator>& lhs,
    const StackAllocator<T2, N, Allocator>& rhs) noexcept
{
    return !(lhs == rhs);
}
