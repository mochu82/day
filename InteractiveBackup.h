#pragma once

#include <string>

class BackupManager;

/**
 * @brief 交互式执行备份操作
 * @param manager 已初始化的备份管理器
 * @return true 表示备份成功；false 表示失败
 */
bool PerformBackupInteractive(BackupManager& manager);

bool performEncryptedBackup(BackupManager& /*manager*/);
// 加密备份函数
bool performEncryptedBackupNonInteractive(
    BackupManager& manager,
    const std::string& sourceDir,
    const std::string& backupDir,
    const std::string& password,
    const std::string& algorithm = "AES-GCM"
);
