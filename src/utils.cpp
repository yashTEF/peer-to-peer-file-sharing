#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include "headers/utils.h"

namespace fs = std::filesystem;

std::string calculate_sha1(const std::string &content) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_length;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(mdctx, content.data(), content.size());
    EVP_DigestFinal_ex(mdctx, hash, &hash_length);
    EVP_MD_CTX_free(mdctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

void write_blob(const std::string &hash, const std::string &content) {
    fs::path blob_file = fs::path(".mygit/objects") / hash;
    if (!fs::exists(blob_file)) {  // Only write if the blob doesn't already exist
        std::ofstream ofs(blob_file, std::ios::binary);
        if (ofs) {
            ofs << content;
        }
    }
}


std::string read_file_content(const fs::path &filepath) {
    std::ifstream file(filepath, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void hash_object(const std::string &filename, bool write) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    std::string hash = calculate_sha1(content);
    
    std::cout << hash << std::endl;

    if (write) {
        write_blob(hash, content);
    }
}

void cat_file(const std::string &flag, const std::string &file_sha) {
    fs::path blob_file = ".mygit/objects/" + file_sha;

    if (!fs::exists(blob_file)) {
        std::cerr << "Error: Object with SHA-1 " << file_sha << " not found." << std::endl;
        return;
    }

    if (flag == "-p") {
        std::ifstream ifs(blob_file, std::ios::binary);
        std::cout << ifs.rdbuf();
    } else if (flag == "-s") {
        std::cout << fs::file_size(blob_file) << " bytes" << std::endl;
    } else if (flag == "-t") {
        std::cout << "blob" << std::endl;  
    } else {
        std::cerr << "Error: Unknown flag " << flag << std::endl;
    }
}

void ls_tree(const std::string &tree_sha, bool name_only) {
    // Retrieve the tree object from storage based on tree_sha
    fs::path tree_file = fs::path(".mygit/objects") / tree_sha;

    std::ifstream ifs(tree_file);
    if (!ifs) {
        std::cerr << "Error: Tree object not found." << std::endl;
        return;
    }

    // Read the tree object data
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string mode, type, sha, name;
        iss >> mode >> type >> sha >> name;

        if (name_only) {
            std::cout << name << std::endl;
        } else {
            std::cout << mode << "\t" << type << "\t" << sha << "\t" << name << std::endl;
        }
    }
}
