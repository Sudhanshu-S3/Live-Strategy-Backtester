/*
 * Copyright 2017-2021, 2023-2024 by timespace management, Flensburg, Germany
 *
 * This file is part of the mio header-only library.
 *
 * See LICENSE for license information.
 */

#ifndef MIO_MMAP_HEADER
#define MIO_MMAP_HEADER

#include <cstdint>
#include <iterator>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define MIO_HAS_EXCEPTIONS
#endif

namespace mio {

// This is used to represent a byte, as `char` is not necessarily a byte.
using byte = unsigned char;

// This is used to represent a mapping's size.
using size_type = std::size_t;

// This is used to represent a mapping's offset.
using offset_type = std::int64_t;

enum class access_mode {
    read,
    write
};

/**
 * This value may be provided as the `mapping_offset` parameter to `map`.
 * It indicates that the logical mapping should be created at a random
 * implementation-defined offset.
 *
 * This value is only supported on Windows.
 */
const offset_type map_entire_file = -1;

namespace detail {

#ifdef _WIN32
using file_handle_type = HANDLE;
const file_handle_type invalid_handle_value = INVALID_HANDLE_VALUE;
#else
using file_handle_type = int;
const file_handle_type invalid_handle_value = -1;
#endif

struct file_handle_wrapper {
    file_handle_type handle = invalid_handle_value;
    bool is_handle_valid() const noexcept { return handle != invalid_handle_value; }
};

} // namespace detail

template <typename ByteT>
class basic_mmap {
public:
    using value_type = ByteT;
    using size_type = mio::size_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using handle_type = detail::file_handle_type;

private:
    // Points to the first requested byte of the mapping.
    pointer data_ = nullptr;

    // The length of the mapping in bytes.
    size_type length_ = 0;

    // The handle of the mapped file.
    handle_type file_handle_ = detail::invalid_handle_value;

#ifdef _WIN32
    // The handle to the windows mapping object.
    handle_type mapping_handle_ = detail::invalid_handle_value;
#endif

public:
    basic_mmap() = default;

    basic_mmap(const std::string& path, const offset_type offset, const size_type length,
               std::error_code& error);

    basic_mmap(const handle_type handle, const offset_type offset, const size_type length,
               std::error_code& error);

    ~basic_mmap() { unmap(); }

    basic_mmap(const basic_mmap&) = delete;
    basic_mmap& operator=(const basic_mmap&) = delete;

    basic_mmap(basic_mmap&& other) noexcept
        : data_(other.data_), length_(other.length_), file_handle_(other.file_handle_)
#ifdef _WIN32
        , mapping_handle_(other.mapping_handle_)
#endif
    {
        other.data_ = nullptr;
        other.length_ = 0;
        other.file_handle_ = detail::invalid_handle_value;
#ifdef _WIN32
        other.mapping_handle_ = detail::invalid_handle_value;
#endif
    }

    basic_mmap& operator=(basic_mmap&& other) noexcept {
        if (this != &other) {
            unmap();
            data_ = other.data_;
            length_ = other.length_;
            file_handle_ = other.file_handle_;
#ifdef _WIN32
            mapping_handle_ = other.mapping_handle_;
#endif
            other.data_ = nullptr;
            other.length_ = 0;
            other.file_handle_ = detail::invalid_handle_value;
#ifdef _WIN32
            other.mapping_handle_ = detail::invalid_handle_value;
#endif
        }
        return *this;
    }

    // Returns whether the mapping was successfully created.
    bool is_open() const noexcept { return data_ != nullptr; }

    // Returns the handle of the underlying file.
    handle_type file_handle() const noexcept { return file_handle_; }

    // Returns a pointer to the first requested byte of the mapping.
    pointer data() noexcept { return data_; }
    const_pointer data() const noexcept { return data_; }

    // Returns the length of the mapping.
    size_type length() const noexcept { return length_; }

    // Returns the length of the mapping.
    size_type size() const noexcept { return length(); }

    // Returns whether the mapping is empty.
    bool empty() const noexcept { return length() == 0; }

    // Returns an iterator to the first element of the mapping.
    iterator begin() noexcept { return data(); }
    const_iterator begin() const noexcept { return data(); }
    const_iterator cbegin() const noexcept { return begin(); }

    // Returns an iterator to one past the last element of the mapping.
    iterator end() noexcept { return data() + length(); }
    const_iterator end() const noexcept { return data() + length(); }
    const_iterator cend() const noexcept { return end(); }

    // Returns a reverse iterator to the first element of the reversed mapping.
    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }

    // Returns a reverse iterator to one past the last element of the reversed mapping.
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return rend(); }

    // Returns a reference to the element at the specified location.
    reference operator[](const size_type i) noexcept { return data_[i]; }
    const_reference operator[](const size_type i) const noexcept { return data_[i]; }

    // Returns a reference to the element at the specified location.
    reference at(const size_type i) {
        if (i >= size()) {
#ifdef MIO_HAS_EXCEPTIONS
            throw std::out_of_range("mio::basic_mmap::at");
#else
            std::abort();
#endif
        }
        return data_[i];
    }

    const_reference at(const size_type i) const {
        if (i >= size()) {
#ifdef MIO_HAS_EXCEPTIONS
            throw std::out_of_range("mio::basic_mmap::at");
#else
            std::abort();
#endif
        }
        return data_[i];
    }

    void unmap();
};

template <typename ByteT>
class basic_mmap_source : public basic_mmap<ByteT> {
public:
    using mmap_type = basic_mmap<ByteT>;
    using handle_type = typename mmap_type::handle_type;

    basic_mmap_source() = default;

    basic_mmap_source(const std::string& path, const offset_type offset, const size_type length,
                      std::error_code& error)
        : mmap_type(path, offset, length, error) {}

    basic_mmap_source(const handle_type handle, const offset_type offset, const size_type length,
                      std::error_code& error)
        : mmap_type(handle, offset, length, error) {}

    void map(const std::string& path, std::error_code& error) {
        map(path, 0, map_entire_file, error);
    }

    void map(const std::string& path, const offset_type offset, const size_type length,
             std::error_code& error);
};

using mmap_source = basic_mmap_source<byte>;

} // namespace mio

#include "mio.ipp"

#endif // MIO_MMAP_HEADER
