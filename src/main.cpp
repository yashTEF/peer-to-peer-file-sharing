#include <iostream>
#include <string>
#include "headers/repository.h"
#include "headers/utils.h"

namespace fs = std::filesystem;

std::string getPathForGit()
{
    fs::path current_path = fs::current_path();
    fs::path mygit_path = current_path / ".mygit";
    return mygit_path;
}

int main(int argc, char *argv[])
{

    fs::path mygit_path = getPathForGit();
    setenv("MYGIT_PATH", mygit_path.c_str(), 1);
    if (argc < 2)
    {
        std::cerr << "Usage: ./mygit <command> [options]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "init")
    {
        if (argc > 2)
        {
            std::cerr << "Error: 'init' takes no additional arguments." << std::endl;
            return 1;
        }
        init_repository();
    }
    else if (command == "hash-object")
    {
        bool write = false;
        std::string filename;

        if (argc == 3)
        {
            filename = argv[2];
        }
        else if (argc == 4 && std::string(argv[2]) == "-w")
        {
            write = true;
            filename = argv[3];
        }
        else
        {
            std::cerr << "Usage: ./mygit hash-object [-w] <file>" << std::endl;
            return 1;
        }

        if (filename.empty())
        {
            std::cerr << "Error: Filename not provided for 'hash-object'." << std::endl;
            return 1;
        }

        hash_object(filename, write);
    }
    else if (command == "cat-file")
    {
        // Ensure exactly 4 arguments: ./mygit cat-file <flag> <file_sha>
        if (argc != 4)
        {
            std::cerr << "Usage: ./mygit cat-file <flag> <file_sha>" << std::endl;
            return 1;
        }

        std::string flag = argv[2];
        std::string hash = argv[3];

        if (flag != "-p" && flag != "-s" && flag != "-t")
        {
            std::cerr << "Error: Invalid flag for 'cat-file'. Use -p, -s, or -t." << std::endl;
            return 1;
        }

        if (hash.empty() || hash.size() != 40)
        {
            std::cerr << "Error: Invalid SHA-1 hash provided." << std::endl;
            return 1;
        }

        cat_file(flag, hash);
    }
    else if (command == "ls-tree")
    {
        bool name_only = false;
        std::string tree_sha;

        if (argc == 3)
        {
            tree_sha = argv[2];
        }
        else if (argc == 4 && std::string(argv[2]) == "--name-only")
        {
            name_only = true;
            tree_sha = argv[3];
        }
        else
        {
            std::cerr << "Usage: ./mygit ls-tree [--name-only] <tree_sha>" << std::endl;
            return 1;
        }

        if (tree_sha.empty() || tree_sha.size() != 40)
        {
            std::cerr << "Error: Invalid SHA-1 hash provided for 'ls-tree'." << std::endl;
            return 1;
        }

        ls_tree(tree_sha, name_only);
    }
    else if (command == "commit")
    {
        std::string message = "Default commit message";

        if (argc == 4 && std::string(argv[2]) == "-m")
        {
            message = argv[3];
        }
        else if (argc != 2)
        {
            std::cerr << "Usage: ./mygit commit [-m <message>]" << std::endl;
            return 1;
        }

        commit(message);
    }
    else if (command == "log")
    {
        log();
    }
    else if (command == "add")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: ./mygit add <file> [<file> ...] or ./mygit add ." << std::endl;
            return 1;
        }

        std::vector<std::string> files(argv + 2, argv + argc);

        if (files.size() == 1 && files[0] == ".")
        {
            // Handle 'git add .'
            fs::path current_dir = fs::current_path();
            std::vector<std::string> all_files;
            for (const auto &entry : fs::directory_iterator(current_dir))
            {
                if (entry.path().string().find(".mygit") != std::string::npos)
                {
                    continue; // Skip this entry
                }
                fs::path relative_path = fs::relative(entry.path(), current_dir);
                all_files.push_back(relative_path.string()); // Add all files and directories
            }
            add_files(all_files); // Add everything in the current directory
        }
        else
        {
            for (const auto &file : files)
            {
                if (file.empty())
                {
                    std::cerr << "Error: Empty filename provided in 'add' command." << std::endl;
                    return 1;
                }
            }
            add_files(files); // Add specified files
        }
    }
    else
    {
        std::cerr << "Error: Unknown command '" << command << "'." << std::endl;
        return 1;
    }

    return 0;
}
