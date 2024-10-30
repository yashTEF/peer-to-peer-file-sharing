#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <filesystem>
#include <vector>

std::string calculate_sha1(const std::string &content);
std::string compress_data(const std::string &data);
std::string decompress_data(const std::string &compressed_data, size_t original_size);
void write_blob(const std::string &hash, const std::string &content);
std::string read_file_content(const std::filesystem::path &filepath);
std::string get_or_create_blob(const std::string &file_content);
void hash_object(const std::string &filename, bool write);
void cat_file(const std::string &flag, const std::string &file_sha);
void ls_tree(const std::string &tree_sha, bool name_only);

#endif // UTILS_H
