#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <system_error>

namespace mio {

template <typename ByteT>
void basic_mmap<ByteT>::unmap() {
    if (!is_open()) {
        return;
    }
#ifdef _WIN32
    UnmapViewOfFile(data_);
    CloseHandle(mapping_handle_);
    CloseHandle(file_handle_);
#else
    munmap(data_, length_);
    close(file_handle_);
#endif
    data_ = nullptr;
    length_ = 0;
    file_handle_ = detail::invalid_handle_value;
#ifdef _WIN32
    mapping_handle_ = detail::invalid_handle_value;
#endif
}

template <typename ByteT>
basic_mmap<ByteT>::basic_mmap(const std::string& path, const offset_type offset, const size_type length,
                               std::error_code& error) {
#ifdef _WIN32
    file_handle_ = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle_ == detail::invalid_handle_value) {
        error.assign(GetLastError(), std::system_category());
        return;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        error.assign(GetLastError(), std::system_category());
        CloseHandle(file_handle_);
        return;
    }

    const size_type total_length = (length == map_entire_file) ? file_size.QuadPart : length;
    mapping_handle_ = CreateFileMapping(file_handle_, 0, PAGE_READONLY, 0, 0, 0);
    if (mapping_handle_ == detail::invalid_handle_value) {
        error.assign(GetLastError(), std::system_category());
        CloseHandle(file_handle_);
        return;
    }

    data_ = static_cast<pointer>(MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, offset, total_length));
    if (data_ == nullptr) {
        error.assign(GetLastError(), std::system_category());
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        return;
    }
    length_ = total_length;
#else
    file_handle_ = open(path.c_str(), O_RDONLY);
    if (file_handle_ == -1) {
        error.assign(errno, std::system_category());
        return;
    }

    struct stat s;
    if (fstat(file_handle_, &s) == -1) {
        error.assign(errno, std::system_category());
        close(file_handle_);
        return;
    }

    const size_type total_length = (length == static_cast<size_type>(map_entire_file)) ? s.st_size : length;
    data_ = static_cast<pointer>(mmap(0, total_length, PROT_READ, MAP_PRIVATE, file_handle_, offset));
    if (data_ == MAP_FAILED) {
        error.assign(errno, std::system_category());
        close(file_handle_);
        return;
    }
    length_ = total_length;
#endif
}

template <typename ByteT>
void basic_mmap_source<ByteT>::map(const std::string& path, const offset_type offset, const size_type length,
                                   std::error_code& error) {
    if (this->is_open()) {
        this->unmap();
    }
    *this = basic_mmap_source(path, offset, length, error);
}

} // namespace mio
