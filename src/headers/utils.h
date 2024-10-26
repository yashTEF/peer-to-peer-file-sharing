#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <filesystem>

std::string calculate_sha1(const std::string &content);
void write_blob(const std::string &hash, const std::string &content);
void hash_object(const std::string &filename, bool write);
void cat_file(const std::string &flag, const std::string &file_sha);
std::string read_file_content(const std::filesystem::path &filepath);
void ls_tree(const std::string &tree_sha, bool name_only);

#endif // UTILS_H
