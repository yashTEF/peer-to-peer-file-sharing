#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <map>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Forward declaration of TreeEntry struct
struct TreeEntry {
    std::string mode;
    std::string name;
    std::string sha;
};

void init_repository();
void add_to_index(const std::string &file_path, const std::string &sha);
void add_file(const fs::path &file_path);
void add_files(const std::vector<std::string> &files);
void create_tree_from_commit(std::map<std::string, std::vector<std::string>>& adjList, const std::string& commit_sha);
void populate_tree_entries(const std::string& tree_sha, std::map<std::string, TreeEntry>& tree_entries);
std::map<std::string, std::vector<std::string>> create_tree_from_index(const std::map<std::string, TreeEntry>& index_entries);
TreeEntry generate_tree_sha(const std::map<std::string, std::vector<std::string>>& adjList, const std::string& current = "");
TreeEntry write_tree();
void log();
void commit(std::string  message);
void checkout(std::string commit_sha);

#endif // REPOSITORY_H
