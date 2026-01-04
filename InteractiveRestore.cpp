// InteractiveRestore.cpp
#include "InteractiveRestore.h"
#include "BackupManager.h"
#include "FilesystemBackupStrategy.h"
#include "ZipExtractor.h"
#include "crypto.h"
#include "PathUtils.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

bool performEncryptedRestore(BackupManager& manager) {
    std::cout << "\n=== 加密备份还原 ===\n";

    std::string encryptedBackup;
    do {
        std::cout << "请输入加密备份文件路径（如 backup.zip.enc）: ";
        std::getline(std::cin, encryptedBackup);
        if (!fs::exists(encryptedBackup)) {
            std::cerr << "错误：备份文件不存在！\n";
        }
    } while (!fs::exists(encryptedBackup));

    // 使用 crypto 中的实现（已在 crypto.cpp 中实现）
    std::string algorithm = selectEncryptionAlgorithm();
    std::string password = getPasswordInput("请输入解密密码: ", false);
    if (password.empty()) return false;

    std::string decryptedZipPath;
    if (!decryptFile(encryptedBackup, password, algorithm, decryptedZipPath)) {
        std::cerr << "❌ 解密失败！\n";
        return false;
    }

    // 使用 PathUtils 的静态方法获取目标路径
    std::string targetDir = PathUtils::GetTargetPath();

    // 创建目标目录（如果不存在）
    fs::create_directories(targetDir);

    std::cout << "正在解压备份内容到: " << targetDir << "\n";
    if (!ZipExtractor::extractToDirectory(decryptedZipPath, targetDir)) {
        std::cerr << "❌ 解压失败！\n";
        fs::remove(decryptedZipPath);
        return false;
    }

    fs::remove(decryptedZipPath); // 清理临时 ZIP
    std::cout << "✅ 还原成功！数据已恢复至: " << targetDir << "\n";
    return true;
}

bool PerformRestoreInteractive(BackupManager& manager) {
    std::cout << "\n选择还原方式:\n";
    std::cout << "1. 普通目录还原（未加密）\n";
    std::cout << "2. 加密备份还原\n";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        std::string backupPath;
        do {
            std::cout << "请输入备份目录路径: ";
            std::getline(std::cin, backupPath);
            if (!fs::exists(backupPath)) {
                std::cout << "路径不存在，请重试。\n";
            }
        } while (!fs::exists(backupPath));

        std::string target = PathUtils::GetTargetPath();
        return manager.performRestore(backupPath, target);
    }
    else if (choice == "2") {
        return performEncryptedRestore(manager);
    }
    else {
        std::cout << "无效选择。\n";
        return false;
    }
}