#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <zlib.h>
#include <iomanip>
#include "headers/utils.h"

namespace fs = std::filesystem;

std::string calculate_sha1(const std::string &content)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_length;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(mdctx, content.data(), content.size());
    EVP_DigestFinal_ex(mdctx, hash, &hash_length);
    EVP_MD_CTX_free(mdctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_length; ++i)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string compress_data(const std::string &data)
{
    uLongf compressed_size = compressBound(data.size());
    std::string compressed_data(compressed_size, '\0');

    if (compress((Bytef *)compressed_data.data(), &compressed_size, (const Bytef *)data.data(), data.size()) != Z_OK)
    {
        std::cerr << "Error compressing data." << std::endl;
        return {};
    }

    compressed_data.resize(compressed_size);
    return compressed_data;
}

std::string decompress_data(const std::string &compressed_data, size_t original_size)
    {
        std::string decompressed_data(original_size, '\0');
        if (uncompress((Bytef *)decompressed_data.data(), &original_size, (const Bytef *)compressed_data.data(), compressed_data.size()) != Z_OK)
        {
            std::cerr << "Error decompressing data." << std::endl;
            return {};
        }
        return decompressed_data;
    }

void write_blob(const std::string &hash, const std::string &content)
{
    fs::path blob_file = fs::path(".mygit/objects") / hash.substr(0, 2) / hash.substr(2);
    if (!fs::exists(blob_file))
    {
        fs::create_directories(blob_file.parent_path());
        std::ofstream ofs(blob_file, std::ios::binary);
        if (ofs)
        {
            ofs << content;
        }
    }
}

std::string read_file_content(const fs::path &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string get_or_create_blob(const std::string &file_content)
{
    std::string object_data = "blob " + std::to_string(file_content.size()) + '\0';
    std::string hash = calculate_sha1(object_data + file_content);

    fs::path blob_dir = fs::path(".mygit/objects") / hash.substr(0, 2);
    fs::path blob_file = blob_dir / hash.substr(2);

    if (fs::exists(blob_file))
    {
        return hash; // Blob already exists
    }

    if (!fs::exists(blob_dir))
    {
        fs::create_directories(blob_dir);
    }
    write_blob(hash, object_data + compress_data(file_content));

    return hash;
}

void hash_object(const std::string &filename, bool write)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::string hash = get_or_create_blob(content);
    std::cout << hash << std::endl;

    if (write)
    {
        write_blob(hash, content);
    }
}

void cat_file(const std::string &flag, const std::string &file_sha)
{
    fs::path blob_file = fs::path(".mygit/objects") / file_sha.substr(0, 2) / file_sha.substr(2);

    if (!fs::exists(blob_file))
    {
        std::cerr << "Error: Object with SHA-1 " << file_sha << " not found." << std::endl;
        return;
    }

    std::ifstream ifs(blob_file, std::ios::binary);
    if (!ifs)
    {
        std::cerr << "Error: Unable to open blob file." << std::endl;
        return;
    }

    std::string compressed_data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    std::size_t null_pos = compressed_data.find('\0');
    if (null_pos == std::string::npos)
    {
        std::cerr << "Error: Null terminator not found in decompressed data." << std::endl;
        return;
    }

    std::string header = compressed_data.substr(0, null_pos);
    std::string content = compressed_data.substr(null_pos + 1);

    std::string type;
    size_t original_size = 0;

    if (header.substr(0, 5) == "blob ")
    {
        type = "blob";
        original_size = std::stoul(header.substr(5));
    }
    else if (header.substr(0, 5) == "tree ")
    {
        type = "tree";
        original_size = std::stoul(header.substr(5));
    }
    else if (header.substr(0, 7) == "commit ")
    {
        type = "commit";
        original_size = std::stoul(header.substr(7));
    }
    else
    {
        std::cerr << "Error: Unknown object type." << std::endl;
        return;
    }
    content = decompress_data(content, original_size);

    if (flag == "-p")
    {
        std::cout << content;
    }
    else if (flag == "-s")
    {
        std::cout << original_size << " bytes" << std::endl;
    }
    else if (flag == "-t")
    {
        std::cout << type << std::endl;
    }
    else
    {
        std::cerr << "Error: Unknown flag " << flag << std::endl;
    }
}

void ls_tree(const std::string &tree_sha, bool name_only)
{
    fs::path tree_file = fs::path(".mygit/objects") / tree_sha.substr(0, 2) / tree_sha.substr(2);

    if (!fs::exists(tree_file))
    {
        std::cerr << "Error: Tree object not found." << std::endl;
        return;
    }

    std::ifstream ifs(tree_file, std::ios::binary);
    std::string object_data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    std::istringstream iss(object_data);
    std::string header;
    std::getline(iss, header, '\0');

    if (header.substr(0, 5) != "tree ")
    {
        std::cerr << "Error: Invalid tree object." << std::endl;
        return;
    }

    size_t original_size = std::stoul(header.substr(5)); // Skip "tree " and parse size
    std::string compressed_content(object_data.begin() + header.size() + 1, object_data.end());
    std::string tree_content = decompress_data(compressed_content, original_size);

    if (tree_content.empty())
    {
        std::cerr << "Error: Decompression failed." << std::endl;
        return;
    }

    std::istringstream tree_iss(tree_content);
    std::string line;

    while (std::getline(tree_iss, line))
    {
        std::istringstream entry_stream(line);
        std::string mode, sha, name;

        entry_stream >> mode >> sha >> name; // Assuming format: <mode> <sha> <name>

        if (name_only)
        {
            std::cout << name << std::endl;
        }
        else
        {
            std::cout << mode << "\t" << sha << "\t" << name << std::endl;
        }
    }
}
