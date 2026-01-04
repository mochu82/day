#pragma once

class BackupManager;

/**
 * @brief 交互式执行还原操作
 * @param manager 已初始化的备份管理器
 * @return true 表示还原成功；false 表示失败
 */
bool PerformRestoreInteractive(BackupManager& manager);
// 解密还原函数
bool performEncryptedRestore(BackupManager& manager);