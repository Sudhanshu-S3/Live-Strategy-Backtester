#ifndef ZSTD_UTILS_H
#define ZSTD_UTILS_H

#include <vector>
#include <string>

namespace zstd_utils {

std::vector<char> compress_file(const std::string& file_path);
void decompress_file(const std::string& compressed_file_path, const std::string& output_file_path);

} // namespace zstd_utils

#endif // ZSTD_UTILS_H
