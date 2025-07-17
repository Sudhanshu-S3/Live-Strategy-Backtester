#include "zstd.h"
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

namespace zstd_utils {

std::vector<char> compress_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Failed to read file: " + file_path);
    }

    size_t const cBuffSize = ZSTD_compressBound(size);
    std::vector<char> compressed_buffer(cBuffSize);

    size_t const cSize = ZSTD_compress(compressed_buffer.data(), cBuffSize, buffer.data(), size, 1);
    if (ZSTD_isError(cSize)) {
        throw std::runtime_error("Compression failed: " + std::string(ZSTD_getErrorName(cSize)));
    }

    compressed_buffer.resize(cSize);
    return compressed_buffer;
}

void decompress_file(const std::string& compressed_file_path, const std::string& output_file_path) {
    std::ifstream file(compressed_file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open compressed file: " + compressed_file_path);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> compressed_buffer(size);
    if (!file.read(compressed_buffer.data(), size)) {
        throw std::runtime_error("Failed to read compressed file: " + compressed_file_path);
    }

    unsigned long long const rSize = ZSTD_getFrameContentSize(compressed_buffer.data(), size);
    if (rSize == ZSTD_CONTENTSIZE_ERROR || rSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("Failed to get decompressed size");
    }

    std::vector<char> decompressed_buffer(rSize);
    size_t const dSize = ZSTD_decompress(decompressed_buffer.data(), rSize, compressed_buffer.data(), size);

    if (ZSTD_isError(dSize) || dSize != rSize) {
        throw std::runtime_error("Decompression failed: " + std::string(ZSTD_getErrorName(dSize)));
    }

    std::ofstream outfile(output_file_path, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Failed to open output file: " + output_file_path);
    }
    outfile.write(decompressed_buffer.data(), dSize);
}

} // namespace zstd_utils
