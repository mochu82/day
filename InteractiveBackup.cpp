#include "InteractiveBackup.h"
#include "BackupManager.h"
#include "PathUtils.h"
#include "ZipCompressor.h"
#include "crypto.h"
#include <iostream>
#include "metadata.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace fs = std::filesystem;
using json = nlohmann::json;

bool PerformBackupInteractive(BackupManager& /*manager*/) {
    std::cout << "\n=== 普通备份 ===" << std::endl;

    // 统一使用 PathUtils 的静态接口
    std::string source = PathUtils::getValidSourcePath();
    std::string dest = PathUtils::GetTargetPath();

    // 其余逻辑不变...
    std::vector<FileMetadata> metadataList;
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        if (fs::is_regular_file(entry)) {
            FileMetadata meta = GetFileMetadata(entry.path().string());
            metadataList.push_back(meta);
        }
    }

    std::cout << "正在备份文件... (" << metadataList.size() << "个文件)" << std::endl;
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        if (fs::is_regular_file(entry)) {
            fs::path destPath = fs::path(dest) / entry.path().filename();
            fs::create_directories(destPath.parent_path());
            fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
        }
    }

    fs::path metadataDir = fs::path(dest) / "backup_metadata";
    fs::create_directories(metadataDir);
    for (const auto& meta : metadataList) {
        fs::path metaFilePath = metadataDir / meta.filename;
        SaveMetadataToFile(metaFilePath.string(), meta);
    }

    std::cout << "备份成功！元数据已保存到: " << metadataDir.string() << "\n";
    return true;
}

// 加密备份函数
bool performEncryptedBackup(BackupManager& /*manager*/) {
    std::cout << "\n=== 加密备份 ===" << std::endl;

    std::string source = PathUtils::getValidSourcePath();
    std::string dest = PathUtils::GetTargetPath();
    std::string password = getPasswordInput("请输入备份密码：", false);
    if (password.empty()) {
        std::cout << "备份已取消。\n";
        return false;
    }

    std::string algorithm = selectEncryptionAlgorithm();

    // 1. 为每个文件收集元数据
    std::vector<FileMetadata> metadataList;
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        if (fs::is_regular_file(entry)) {
            FileMetadata meta = GetFileMetadata(entry.path().string());
            metadataList.push_back(meta);
        }
    }

    // 2. 压缩为ZIP
    fs::path tempZipPath = fs::path(dest) / "temp_backup.zip";
    std::cout << "正在压缩备份文件..." << std::endl;
    if (!ZipCompressor::compressDirectory(source, tempZipPath.string())) {
        std::cerr << "压缩失败！" << std::endl;
        return false;
    }

    // 3. 保存元数据到单独 JSON 文件（在加密前）
    fs::path metadataFilePath = fs::path(dest) / "backup_metadata.json";
    std::ofstream metaFile(metadataFilePath.string());
    if (!metaFile) {
        std::cerr << "无法创建元数据文件！" << std::endl;
        // 清理临时 ZIP
        std::remove(tempZipPath.string().c_str());
        return false;
    }

    json j;
    for (const auto& meta : metadataList) {
        j[meta.filename] = {
            {"creationTime", {meta.creationTime.dwLowDateTime, meta.creationTime.dwHighDateTime}},
            {"lastAccessTime", {meta.lastAccessTime.dwLowDateTime, meta.lastAccessTime.dwHighDateTime}},
            {"lastWriteTime", {meta.lastWriteTime.dwLowDateTime, meta.lastWriteTime.dwHighDateTime}},
            {"fileSize", meta.fileSize},
            {"fileAttributes", meta.fileAttributes}
        };
    }
    metaFile << j.dump(4);
    metaFile.close();

    // 4. 将元数据文件添加到 ZIP
    // 注意：当前代码库没有通用的“向已有 ZIP 添加文件”的实现。
    // 在生产代码中，应使用 minizip 或类似库将 metadataFilePath 添加到 tempZipPath。
    std::cout << "将元数据文件添加到ZIP... (请在 ZIP 库中实现实际添加逻辑)" << std::endl;

    // 5. 加密ZIP文件
    std::string encryptedPath;
    if (!encryptZipFile(tempZipPath.string(), password, algorithm, encryptedPath)) {
        std::remove(tempZipPath.string().c_str());
        std::remove(metadataFilePath.string().c_str());
        return false;
    }

    // 6. 删除临时文件
    std::remove(tempZipPath.string().c_str());
    std::remove(metadataFilePath.string().c_str());

    // 7. 重命名加密文件
    // 修复 E2140 错误：三元表达式的类型问题，需将字符串拼接移出括号
    std::string ext = (algorithm == "AES-GCM") ? "aes" : "cha";
    fs::path finalBackupPath = fs::path(dest) / ("backup." + ext);
    std::rename(encryptedPath.c_str(), finalBackupPath.string().c_str());

    std::cout << "备份完成！加密文件保存在：" << finalBackupPath.string() << std::endl;
    std::cout << "加密算法：" << algorithm << std::endl;
    return true;
}
bool performEncryptedBackupNonInteractive(
    BackupManager& /*manager*/, // 暂未使用，保留接口一致性
    const std::string& sourceDir,
    const std::string& backupDir,
    const std::string& password,
    const std::string& algorithm)
{
    namespace fs = std::filesystem;

    if (!fs::exists(sourceDir)) {
        std::cerr << "Source directory does not exist: " << sourceDir << "\n";
        return false;
    }

    if (!fs::exists(backupDir)) {
        if (!fs::create_directories(backupDir)) {
            std::cerr << "Failed to create backup directory: " << backupDir << "\n";
            return false;
        }
    }

    // 生成带时间戳的 ZIP 文件名
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss; // 修复未初始化本地变量和不完整类型

    // 使用线程安全的本地时间函数（在 MSVC 使用 localtime_s，其他平台使用 localtime_r）
    std::tm local_tm;
    #if defined(_MSC_VER)
        localtime_s(&local_tm, &time_t);
    #else
        localtime_r(&time_t, &local_tm);
    #endif

    ss << std::put_time(&local_tm, "%Y%m%d_%H%M%S");
    std::string timestamp = ss.str();

    std::string zipPath = backupDir + "/backup_" + timestamp + ".zip";
    std::string encryptedPath = zipPath + ".enc";

    // 1. 压缩目录
    std::cout << "Compressing to: " << zipPath << "\n";
    if (!ZipCompressor::compressDirectory(sourceDir, zipPath)) {
        std::cerr << "Compression failed.\n";
        return false;
    }

    // 2. 加密 ZIP 文件
    std::cout << "Encrypting...\n";
    if (!encryptZipFile(zipPath, password, algorithm, encryptedPath)) {
        std::cerr << "Encryption failed.\n";
        std::remove(zipPath.c_str());
        return false;
    }

    // 3. 删除临时 ZIP
    std::remove(zipPath.c_str());

    std::cout << "✅ Encrypted backup saved to: " << encryptedPath << "\n";
    return true;
}