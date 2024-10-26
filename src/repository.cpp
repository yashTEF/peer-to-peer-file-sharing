#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include "headers/repository.h"
#include "headers/utils.h"

namespace fs = std::filesystem;

void init_repository() {
    fs::path mygit_dir(".mygit");
    
    if (!fs::exists(mygit_dir)) {
        fs::create_directory(mygit_dir);
        fs::create_directory(mygit_dir / "objects");
        fs::create_directory(mygit_dir / "refs");
        fs::create_directory(mygit_dir / "refs/heads");
        std::cout << ".mygit directory initialized." << std::endl;
    } else {
        std::cout << ".mygit directory already exists." << std::endl;
    }
}

std::string create_tree_object(const fs::path &directory) {
    std::ostringstream tree_data;

    for (const auto &entry : fs::directory_iterator(directory)) {
        std::string hash;
        if (entry.is_directory()) {
            hash = create_tree_object(entry.path());  // Recursive for subdirectories
            tree_data << "40000 " << entry.path().filename().string() << '\0' << hash;
        } else if (entry.is_regular_file()) {
            std::string file_content = read_file_content(entry.path());
            hash = calculate_sha1("blob " + std::to_string(file_content.size()) + '\0' + file_content);
            tree_data << "100644 " << entry.path().filename().string() << '\0' << hash;
        }
    }

    std::string tree_content = "tree " + std::to_string(tree_data.str().size()) + '\0' + tree_data.str();
    std::string tree_hash = calculate_sha1(tree_content);
    write_blob(tree_hash, tree_content);
    
    return tree_hash;
}

void write_tree() {
    fs::path current_dir = fs::current_path();
    std::string tree_hash = create_tree_object(current_dir);
    std::cout << "Tree object created with SHA-1 hash: " << tree_hash << std::endl;
}
