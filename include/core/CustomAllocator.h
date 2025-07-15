#ifndef CUSTOM_ALLOCATOR_H
#define CUSTOM_ALLOCATOR_H

#include <cstddef>
#include <new>
#include <vector>

template <typename T>
class PoolAllocator {
public:
    using value_type = T;

    PoolAllocator(size_t pool_size = 1024) : pool_size_(pool_size) {}

    template <typename U>
    PoolAllocator(const PoolAllocator<U>&) noexcept {}

    T* allocate(size_t n) {
        if (n > 1 || free_list_.empty()) {
            if (n > 1) {
                return static_cast<T*>(::operator new(n * sizeof(T)));
            }
            // Allocate a new pool
            T* new_pool = static_cast<T*>(::operator new(pool_size_ * sizeof(T)));
            pools_.push_back(new_pool);
            for (size_t i = 0; i < pool_size_; ++i) {
                free_list_.push_back(&new_pool[i]);
            }
        }
        T* ptr = free_list_.back();
        free_list_.pop_back();
        return ptr;
    }

    void deallocate(T* p, size_t n) {
        if (n > 1) {
            ::operator delete(p);
            return;
        }
        free_list_.push_back(p);
    }

private:
    size_t pool_size_;
    std::vector<T*> pools_;
    std::vector<T*> free_list_;
};

template <typename T, typename U>
bool operator==(const PoolAllocator<T>&, const PoolAllocator<U>&) {
    return true;
}

template <typename T, typename U>
bool operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&) {
    return false;
}

#endif // CUSTOM_ALLOCATOR_H
