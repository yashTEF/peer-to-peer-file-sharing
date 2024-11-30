#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <zlib.h>
#include <iomanip>
#include <map>
#include <ctime>
#include <algorithm>
#include "headers/repository.h"
#include "headers/utils.h"
#include <queue>

namespace fs = std::filesystem;

struct Commit
{
    std::string tree_sha;
    std::string parent_sha;
    std::string message;
    std::string author;
    std::string committer;
    std::string timestamp;

    std::string serialize() const
    {
        std::ostringstream oss;
        oss << "tree " << tree_sha << "\n";
        if (!parent_sha.empty())
        {
            oss << "parent " << parent_sha << "\n";
        }
        oss << "author " << author << " " << timestamp << "\n";
        oss << "committer " << committer << " " << timestamp << "\n";
        oss << "\n"
            << message << "\n";
        return oss.str();
    }
};

void init_repository()
{
    fs::create_directories(".mygit/objects");
    std::ofstream index_file(".mygit/index");
    fs::create_directories(".mygit/refs/heads");
    std::ofstream master_file(".mygit/refs/heads/master");
    master_file.close();
    index_file.close();
}

void add_to_index(const std::string &file_path, const std::string &sha, const std::string &mode)
{
    std::ofstream index_file(".mygit/index", std::ios::app);
    if (index_file)
    {
        index_file << mode << " " << file_path << " " << sha << "\n";
    }
}

std::string get_file_mode(const fs::path &file_path)
{
    auto status = fs::status(file_path);
    if (fs::is_regular_file(status))
    {
        return "100644"; // Regular file mode
    }
    else if (fs::is_directory(status))
    {
        return "40000"; // Directory mode
    }
    // Additional modes can be added as needed
    return "000000"; // Unknown or unsupported file type
}

void add_file(const fs::path &file_path)
{
    std::string file_content = read_file_content(file_path);
    std::string sha = get_or_create_blob(file_content);
    std::string mode = get_file_mode(file_path);
    add_to_index(file_path.string(), sha, mode);
}

void add_files(const std::vector<std::string> &files)
{
    for (const auto &file : files)
    {
        fs::path file_path(file);
        if (fs::exists(file_path) && fs::is_regular_file(file_path))
        {
            add_file(file_path);
        }
        else if (fs::is_directory(file_path))
        {
            for (auto &p : fs::recursive_directory_iterator(file_path))
            {
                if (fs::is_regular_file(p.path()))
                {
                    add_file(p.path());
                }
            }
        }
        else
        {
            std::cerr << "Warning: " << file << " not found.\n";
        }
    }
}

std::map<std::string, TreeEntry> read_from_index()
{
    std::map<std::string, TreeEntry> index_entries;
    std::ifstream index_file(".mygit/index");

    std::string mode, path, sha;
    while (index_file >> mode >> path >> sha)
    {
        index_entries[path] = {mode, path, sha};
    }

    return index_entries;
}

std::map<std::string, std::vector<std::string>> create_tree_from_index(const std::map<std::string, TreeEntry> &index_entries, std::map<std::string, std::vector<std::string>> &adjList)
{
    if (adjList.find("") == adjList.end())
    {
        adjList[""] = {};
    }

    for (const auto &[path, entry] : index_entries)
    {
        std::string current_path = "";
        std::istringstream path_stream(path);
        std::string component;

        while (std::getline(path_stream, component, '/'))
        {
            std::string next_path = current_path.empty() ? component : current_path + "/" + component;
            auto &children = adjList[current_path];
            if (std::find(children.begin(), children.end(), next_path) == children.end())
            {
                children.push_back(next_path);
            }
            current_path = next_path;
        }

        if (adjList.find(current_path) == adjList.end())
        {
            adjList[current_path] = {};
        }
    }

    for (const auto &[path, entry] : index_entries)
    {
        if (adjList.find(path) == adjList.end())
        {
            adjList[path] = {};
        }
    }

    return adjList;
}
std::map<std::string, TreeEntry> read_tree(const std::string &tree_sha)
{
    std::map<std::string, TreeEntry> tree_entries;
    populate_tree_entries(tree_sha, tree_entries);
    return tree_entries;
}

void populate_tree_entries(const std::string &tree_sha, std::map<std::string, TreeEntry> &tree_entries)
{
    std::stringstream buffer;
    std::streambuf *old_buf = std::cout.rdbuf(buffer.rdbuf());

    cat_file("-p", tree_sha);

    std::cout.rdbuf(old_buf);

    std::string line;
    while (std::getline(buffer, line))
    {
        std::istringstream iss(line);
        std::string mode, name, sha;
        iss >> mode >> name >> sha;

        tree_entries[name] = {mode, name, sha};

        if (mode == "040000")
        {
            populate_tree_entries(sha, tree_entries);
        }
    }
}

std::string get_tree_sha_from_commit(const std::string &commit_sha)
{
    std::stringstream buffer;
    std::streambuf *old_buf = std::cout.rdbuf(buffer.rdbuf());

    cat_file("-p", commit_sha);

    std::cout.rdbuf(old_buf);

    std::string line;
    std::string tree_sha;

    while (std::getline(buffer, line))
    {
        if (line.find("tree ") == 0)
        {
            tree_sha = line.substr(5);
            break;
        }
    }

    return tree_sha;
}
void create_tree_from_commit(std::map<std::string, std::vector<std::string>> &adjList, const std::string &commit_sha)
{
    std::string tree_sha = get_tree_sha_from_commit(commit_sha);
    if (tree_sha.empty())
    {
        return;
    }

    std::map<std::string, TreeEntry> tree_entries = read_tree(tree_sha);
    create_tree_from_index(tree_entries, adjList);
}

TreeEntry generate_tree_sha(const std::map<std::string, std::vector<std::string>> &adjList,
                            const std::map<std::string, TreeEntry> &tree_entries,
                            const std::string &current)
{
    std::ostringstream serialized_tree;

    // std::cout << "Adjacency List:" << std::endl;
    // for (const auto &pair : adjList) {
    //     std::cout << pair.first << ": ";
    //     for (const auto &child : pair.second) {
    //         std::cout << child << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Generating tree SHA for: " << current << std::endl;

    // Iterate through the children of the current directory
    for (const auto &child : adjList.at(current))
    {
        std::cout << "  Processing child: " << child << std::endl;

        // Check if the child is a file or a directory
        if (adjList.at(child).empty())
        {
            // It's a file (leaf), retrieve its SHA from tree_entries
            auto it = tree_entries.find(child);
            if (it != tree_entries.end())
            {
                std::string blob_sha = it->second.sha;                            // Correctly retrieve the SHA of the file
                serialized_tree << "100644 " << child << " " << blob_sha << '\n'; // Blob entry

                // Debugging output for files
                std::cout << "    Found file: " << child << " with SHA: " << blob_sha << std::endl;
            }
            else
            {
                std::cerr << "Error: Leaf file not found in tree_entries: " << child << std::endl;
                throw std::runtime_error("Leaf file not found in tree_entries: " + child);
            }
        }
        else
        {
            // It's a directory
            TreeEntry child_entry = generate_tree_sha(adjList, tree_entries, child);

            // Debugging output for directories
            std::cout << "    Found directory: " << child_entry.name << " with SHA: " << child_entry.sha << std::endl;

            // Append directory information to serialized tree representation
            serialized_tree << "040000 " << child_entry.name << " " << child_entry.sha << '\n'; // Directory entry
        }
    }

    // Only create the tree object if the current entry is a directory
    if (!adjList.at(current).empty())
    {
        // Serialize the tree data after processing all children
        std::string serialized_data = serialized_tree.str();
        std::string tree_object = "tree " + std::to_string(serialized_data.size()) + '\0';
        std::string tree_sha = calculate_sha1(tree_object + serialized_data); // Calculate SHA for the directory tree

        // Debugging output for the tree object
        std::cout << "  Writing tree object for directory: " << current << " with SHA: " << tree_sha << std::endl;

        // Write the tree object to the object store
        write_blob(tree_sha, tree_object + compress_data(serialized_data));

        return {"040000", current, tree_sha}; // Returns the current directory's TreeEntry
    }

    // If current is a directory with no children, return an empty TreeEntry
    std::cout << "  Current directory is empty: " << current << std::endl;
    return {"040000", current, ""}; // Returning an empty sha for empty directories
}

TreeEntry write_tree()
{
    std::map<std::string, std::vector<std::string>> adjList;
    std::map<std::string, TreeEntry> tree_entries;

    // Read parent commit SHA if available
    std::ifstream parent_file(".mygit/refs/heads/master");
    std::string parent_sha;
    if (parent_file)
    {
        std::getline(parent_file, parent_sha);
        if (!parent_sha.empty())
            create_tree_from_commit(adjList, parent_sha); // Populate adjList from the commit tree
    }

    // Debug output for parent SHA
    std::cout << "Parent SHA: " << (parent_sha.empty() ? "None" : parent_sha) << std::endl;

    if (!parent_sha.empty())
    {
        tree_entries = read_tree(get_tree_sha_from_commit(parent_sha)); // Read tree entries from the commit
    }

    // Debug output for tree entries after reading parent
    std::cout << "Tree Entries after reading parent commit:" << std::endl;
    for (const auto &entry : tree_entries)
    {
        std::cout << entry.first << " -> Mode: " << entry.second.mode
                  << ", SHA: " << entry.second.sha << std::endl;
    }

    // Read index entries
    auto index_entries = read_from_index();

    // Debug output for index entries
    // std::cout << "Index Entries:" << std::endl;
    // for (const auto &entry : index_entries)
    // {
    //     std::cout << entry.first << " -> Mode: " << entry.second.mode
    //               << ", SHA: " << entry.second.sha << std::endl;
    // }

    // Populate tree_entries with index entries
    for (const auto &entry : index_entries)
    {
        tree_entries[entry.first] = {entry.second.mode, entry.first, entry.second.sha};
    }

    // Debug output for tree_entries after updating with index entries
    // std::cout << "Tree Entries after updating with index entries:" << std::endl;
    // for (const auto &entry : tree_entries)
    // {
    //     std::cout << entry.first << " -> Mode: " << entry.second.mode
    //               << ", SHA: " << entry.second.sha << std::endl;
    // }

    // Build adjList from index entries
    create_tree_from_index(index_entries, adjList);

    // Debug output for adjacency list
    std::cout << "Adjacency List:" << std::endl;
    for (const auto &pair : adjList)
    {
        std::cout << pair.first << ": ";
        for (const auto &child : pair.second)
        {
            std::cout << child << " ";
        }
        std::cout << std::endl;
    }

    // Generate the final tree SHA using the combined tree_entries
    TreeEntry rootEntry = generate_tree_sha(adjList, tree_entries, ""); // Pass tree_entries to generate_tree_sha
    return rootEntry;
}


void log()
{
    std::ifstream head_file(".mygit/refs/heads/master");
    if (!head_file)
    {
        std::cerr << "Error: HEAD not found." << std::endl;
        return;
    }

    std::string commit_sha;
    std::getline(head_file, commit_sha);

    while (!commit_sha.empty())
    {
        std::string commit_content;
        std::ostringstream oss;
        std::streambuf *original_cout_buffer = std::cout.rdbuf(oss.rdbuf());

        cat_file("-p", commit_sha);
        std::cout.rdbuf(original_cout_buffer);

        commit_content = oss.str();
        std::istringstream commit_stream(commit_content);
        std::string line;
        std::string parent_sha;
        std::string message;
        std::string author;
        std::string committer;
        std::string tree_sha;

        while (std::getline(commit_stream, line))
        {
            if (line.find("tree ") == 0)
            {
                tree_sha = line.substr(5);
            }
            else if (line.find("parent ") == 0)
            {
                parent_sha = line.substr(7);
            }
            else if (line.find("author ") == 0)
            {
                author = line.substr(7);
            }
            else if (line.find("committer ") == 0)
            {
                committer = line.substr(11);
            }
            else if (line.empty())
            {
                continue;
            }
            else
            {
                message += line + "\n";
            }
        }
        std::cout << "commit " << commit_sha << "\n";
        std::cout << "tree " << tree_sha << "\n";
        if (!parent_sha.empty())
        {
            std::cout << "parent " << parent_sha << "\n";
        }
        std::cout << "author " << author << "\n";
        std::cout << "committer " << committer << "\n";
        std::cout << "\n"
                  << message << "\n";
        std::cout << "------------------------------------\n";

        commit_sha = parent_sha;
    }
}

void commit(std::string message)
{
    std::string tree_sha = write_tree().sha;
    std::string parent_sha;

    std::ifstream parent_file(".mygit/refs/heads/master");
    if (parent_file)
    {
        std::getline(parent_file, parent_sha);
    }

    std::string author = "Your Name <you@example.com>";
    std::string committer = author;

    std::time_t now = std::time(nullptr);
    std::ostringstream timestamp_stream;
    timestamp_stream << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S %z");
    std::string timestamp = timestamp_stream.str();

    std::ostringstream serialized_commit;
    serialized_commit << "tree " << tree_sha << "\n";
    if (!parent_sha.empty())
    {
        serialized_commit << "parent " << parent_sha << "\n";
    }
    serialized_commit << "author " << author << " " << timestamp << "\n";
    serialized_commit << "committer " << committer << " " << timestamp << "\n";
    serialized_commit << "\n"
                      << message << "\n";

    std::string serialized_data = serialized_commit.str();
    std::string commit_object = "commit " + std::to_string(serialized_data.size()) + '\0';
    std::string commit_sha = calculate_sha1(commit_object + serialized_data);

    write_blob(commit_sha, commit_object + compress_data(serialized_data));

    std::ofstream head_file(".mygit/refs/heads/master");
    if (head_file)
    {
        head_file << commit_sha << std::endl;
    }
    std::ofstream index_file(".mygit/index", std::ios::trunc);
    if (index_file)
    {
        index_file.close();
    }
    std::cout << "Committed: " << commit_sha << std::endl;
    std::cout << serialized_data;
}

bool has_uncommitted_changes()
{
    auto index_entries = read_from_index();

    // If the index is empty, return false (no uncommitted changes)
    return !index_entries.empty();
}

void restore_blob(const std::string &blob_sha, const std::string &file_path)
{
    std::stringstream buffer;
    std::streambuf *old_buf = std::cout.rdbuf(buffer.rdbuf());

    cat_file("-p", blob_sha);

    std::cout.rdbuf(old_buf);

    std::string content = buffer.str();
    std::ofstream file(file_path);
    if (file)
    {
        file << content;
    }
    else
    {
        throw std::runtime_error("Error: Unable to create file " + file_path);
    }
}

void clear_project_directory(const fs::path &project_root)
{
    try
    {
        for (const auto &entry : fs::directory_iterator(project_root))
        {
            // Check if the current entry is the .mygit directory
            if (entry.path().filename() != ".mygit")
            {
                fs::remove_all(entry.path()); // Remove files and directories
            }
        }
        std::cout << "Cleared project directory, preserving .mygit." << std::endl;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Error clearing project directory: " << e.what() << std::endl;
    }
}

void restore_tree(const std::string &tree_sha)
{
    std::map<std::string, TreeEntry> tree_entries = read_tree(tree_sha);

    for (const auto &[file_name, entry] : tree_entries)
    {
        std::cout << "Restoring: " << file_name << " (SHA: " << entry.sha << ", Mode: " << entry.mode << ")\n";

        if (entry.mode == "100644") // Regular file
        {
            // Restore the file from the blob using its SHA
            restore_blob(entry.sha, file_name);
            std::cout << "Restored file: " << file_name << std::endl;
        }
        else if (entry.mode == "040000") // Directory
        {
            // Create directory if it doesn't exist
            fs::create_directory(file_name);
            std::cout << "Created directory: " << file_name << std::endl;

            // If the entry is a directory, we need to restore its tree as well
            // This might require reading the entries under this directory
            // Here, you can call a function to restore the directory contents, if needed.
        }
        // Add additional handling for other modes if necessary
    }
}

void checkout(std::string commit_sha)
{
    std::ifstream head_file(".mygit/refs/heads/master");
    if (!head_file)
    {
        std::cerr << "Error: HEAD not found." << std::endl;
        return;
    }

    std::string current_commit_sha;
    std::getline(head_file, current_commit_sha);

    std::string commit_tree_sha = get_tree_sha_from_commit(commit_sha);
    if (commit_tree_sha.empty())
    {
        std::cerr << "Error: Invalid commit SHA." << std::endl;
        return;
    }

    std::cout << "Checking out commit " << commit_sha << std::endl;
    clear_project_directory(".");
    // Restore the tree at the specified commit
    restore_tree(commit_tree_sha);

    std::ofstream head_file_out(".mygit/refs/heads/master");
    if (head_file_out)
    {
        head_file_out << commit_sha << std::endl; // Update HEAD
    }

    std::cout << "Successfully checked out to commit: " << commit_sha << std::endl;
}
