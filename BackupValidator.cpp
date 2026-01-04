#include "BackupValidator.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// 比较两个文件内容是否完全相同
static bool filesEqual(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);

    if (!f1.is_open() || !f2.is_open()) {
        return false;
    }

    // 先比较大小（快速失败）
    f1.seekg(0, std::ios::end);
    f2.seekg(0, std::ios::end);
    if (f1.tellg() != f2.tellg()) {
        return false;
    }
    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);

    // 分块比较内容
    const size_t bufferSize = 65536; // 64KB
    std::vector<char> buffer1(bufferSize), buffer2(bufferSize);

    while (true) {
        f1.read(buffer1.data(), bufferSize);
        f2.read(buffer2.data(), bufferSize);
        std::streamsize count1 = f1.gcount();
        std::streamsize count2 = f2.gcount();

        if (count1 != count2) return false;
        if (count1 == 0) break; // 文件结束

        if (std::memcmp(buffer1.data(), buffer2.data(), count1) != 0) {
            return false;
        }
    }

    return true;
}

// 递归收集目录中所有文件的相对路径（仅文件，不含目录）
static void collectFiles(const fs::path& root, std::vector<fs::path>& files) {
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            files.push_back(fs::relative(entry.path(), root));
        }
    }
}

bool BackupValidator::validate(const std::string& sourceDir, const std::string& backupDir) {
    if (!fs::exists(sourceDir) || !fs::exists(backupDir)) {
        return false;
    }

    if (!fs::is_directory(sourceDir) || !fs::is_directory(backupDir)) {
        return false;
    }

    std::vector<fs::path> sourceFiles, backupFiles;
    collectFiles(sourceDir, sourceFiles);
    collectFiles(backupDir, backupFiles);

    // 快速检查：文件数量是否一致
    if (sourceFiles.size() != backupFiles.size()) {
        return false;
    }

    // 转为 set 便于查找（或排序后一一对应）
    std::sort(sourceFiles.begin(), sourceFiles.end());
    std::sort(backupFiles.begin(), backupFiles.end());

    if (sourceFiles != backupFiles) {
        return false; // 文件名/结构不一致
    }

    // 逐个比对内容
    for (const auto& relPath : sourceFiles) {
        fs::path srcFile = fs::path(sourceDir) / relPath;
        fs::path bakFile = fs::path(backupDir) / relPath;

        if (!filesEqual(srcFile.string(), bakFile.string())) {
            return false;
        }
    }

    return true;
}