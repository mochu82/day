#include "BatchZipCompressor.h"
#include "ZipCompressor.h"
#include "PathUtils.h"
#include <minizip/zip.h>
#include <iostream>
#include <filesystem>
#include <queue>
#include <fstream>

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

namespace fs = std::filesystem;

// 辅助：将单个文件添加到 zip
static bool addFileToZip(zipFile& zip, const std::string& filePath, const std::string& archivePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return false;

    zip_fileinfo zi = { 0 };
    if (zipOpenNewFileInZip(
        zip,
        archivePath.c_str(),
        &zi,
        nullptr, 0, nullptr, 0,
        nullptr,
        Z_DEFLATED,
        Z_DEFAULT_COMPRESSION) != ZIP_OK) {
        return false;
    }

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (zipWriteInFileInZip(zip, buffer, static_cast<unsigned int>(file.gcount())) != ZIP_OK) {
            zipCloseFileInZip(zip);
            return false;
        }
    }

    zipCloseFileInZip(zip);
    return true;
}

// 辅助：递归添加目录到 zip（带相对路径）
static bool addDirectoryToZip(zipFile& zip, const std::string& rootPath, const std::string& currentPath) {
    for (const auto& entry : fs::directory_iterator(currentPath)) {
        std::string relativePath = fs::relative(entry.path(), rootPath).string();
#ifdef _WIN32
        for (char& c : relativePath) if (c == '\\') c = '/';
#endif

        if (entry.is_directory()) {
            // 确保目录以 / 结尾
            if (!relativePath.empty() && relativePath.back() != '/') relativePath += '/';
            zip_fileinfo zi = { 0 };
            if (zipOpenNewFileInZip(zip, relativePath.c_str(), &zi, nullptr, 0, nullptr, 0, nullptr, 0, 0) != ZIP_OK)
                return false;
            zipCloseFileInZip(zip);
            if (!addDirectoryToZip(zip, rootPath, entry.path().string()))
                return false;
        }
        else {
            if (!addFileToZip(zip, entry.path().string(), relativePath))
                return false;
        }
    }
    return true;
}

bool BatchZipCompressor::batchCompress(const std::vector<std::string>& sourceDirs, const std::string& outputZipPath) {
    if (sourceDirs.empty()) {
        std::cerr << "? 未提供任何源目录。\n";
        return false;
    }

    // 创建输出 ZIP 的父目录
    size_t lastSlash = outputZipPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string parentDir = outputZipPath.substr(0, lastSlash);
        if (!parentDir.empty()) {
            std::error_code ec;
            fs::create_directories(parentDir, ec);
            if (ec) {
                std::cerr << "? 无法创建 ZIP 输出目录: " << parentDir << "\n";
                return false;
            }
        }
    }

    zipFile zip = zipOpen(outputZipPath.c_str(), APPEND_STATUS_CREATE);
    if (!zip) {
        std::cerr << "? 无法创建 ZIP 文件: " << outputZipPath << "\n";
        return false;
    }

    bool success = true;
    for (const auto& dir : sourceDirs) {
        if (!fs::exists(dir)) {
            std::cerr << "?? 跳过不存在的目录: " << dir << "\n";
            continue;
        }
        if (!fs::is_directory(dir)) {
            std::cerr << "?? 跳过非目录项: " << dir << "\n";
            continue;
        }

        std::cout << "?? 打包目录: " << dir << "\n";
        if (!addDirectoryToZip(zip, dir, dir)) {
            std::cerr << "? 打包失败: " << dir << "\n";
            success = false;
        }
    }

    zipClose(zip, nullptr);
    if (success) {
        std::cout << "? 批量打包成功: " << outputZipPath << "\n";
    }
    return success;
}