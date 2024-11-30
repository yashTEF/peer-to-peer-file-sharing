# MyGit Project
This is a mini version control system implemented in C++.

# Mini Version Control System (mygit)

## Overview
This is a simple implementation of a version control system (VCS) in C++. It provides basic functionalities similar to Git, allowing users to manage project files, track changes, and navigate through different versions of the project. The system is designed to be lightweight and straightforward, making it an excellent educational tool for understanding the core principles behind version control systems. 

The project supports basic operations to help users maintain and track changes to their files, making it easier to collaborate on projects and revert to previous states as needed. This implementation also introduces concepts such as blobs, trees, and commits, giving users insights into how popular VCS like Git operate under the hood.

## Features
- **Initialize a new repository (`init`)**: Create a new version control repository in the current directory, setting up the necessary file structure and metadata to start tracking changes.

- **Add files and directories to the staging area (`add`)**: Stage specific files or entire directories for the next commit. Users can add individual files or use a wildcard to add all changes in the current directory.

- **Commit changes with a message (`commit`)**: Record the staged changes in the repository's history, along with a user-defined commit message. Each commit is associated with a unique identifier (SHA) for easy reference.

- **Checkout specific commits (`checkout`)**: Revert the working directory to a previous state by checking out a specific commit. This allows users to view or restore the contents of their project as it was at the time of that commit.

- **Display commit history (`log`)**: View a chronological list of all commits made in the repository, including details like commit SHA, message, and timestamp, enabling users to track the evolution of their project.

- **Restore files from previous commits (`cat_file`)**: Retrieve the content of a specific file as it was in a previous commit, allowing users to access older versions of files directly.

- **Basic Error Handling**: The system includes error handling to manage common issues, such as attempting to checkout a non-existent commit or trying to add files that are not tracked.

This mini VCS project serves as a practical example of how version control systems function and provides a foundation for further enhancements, such as branching, merging, and conflict resolution.
    