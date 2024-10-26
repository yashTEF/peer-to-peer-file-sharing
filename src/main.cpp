#include <iostream>
#include <string>
#include "headers/repository.h"
#include "headers/utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./mygit <command> [options]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        init_repository();
    } else if (command == "hash-object") {
        bool write = (argc > 3 && std::string(argv[2]) == "-w");
        hash_object(argv[argc - 1], write);
    } else if (command == "cat-file") {
        if (argc != 4) {
            std::cerr << "Usage: ./mygit cat-file <flag> <file_sha>" << std::endl;
            return 1;
        }
        std::string flag = argv[2];
        std::string hash = argv[3];
        cat_file(flag, hash);
    } else if (command == "ls-tree") {
        bool name_only = (argc > 2 && std::string(argv[2]) == "--name-only");
        std::string tree_sha = argv[name_only ? 3 : 2];
        ls_tree(tree_sha, name_only);
    } else {
        std::cerr << "Error: Unknown command " << command << std::endl;
    }

    return 0;
}
