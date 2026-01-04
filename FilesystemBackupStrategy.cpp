#include "FilesystemBackupStrategy.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

bool FilesystemBackupStrategy::copyRecursively(const std::string& src, const std::string& dst) {
    try {
        if (!fs::exists(src)) {
            std::cerr << "Error: Source path does not exist: " << src << "\n";
            return false;
        }

        fs::create_directories(dst);

        for (const auto& entry : fs::recursive_directory_iterator(src)) {
            auto relativePath = entry.path().lexically_relative(src);
            auto targetPath = fs::path(dst) / relativePath;

            if (fs::is_directory(entry)) {
                fs::create_directories(targetPath);
            }
            else {
                fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
                std::cout << "Copied: " << entry.path().string() << "\n";
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during copy: " << e.what() << "\n";
        return false;
    }
}

bool FilesystemBackupStrategy::backup(const std::string& source, const std::string& dest) {
    std::cout << "Starting backup...\n";
    return copyRecursively(source, dest);
}

bool FilesystemBackupStrategy::restore(const std::string& backupPath, const std::string& target) {
    std::cout << "Starting restore...\n";
    return copyRecursively(backupPath, target);
}