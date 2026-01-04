#include "BackupManager.h"
#include <iostream>

BackupManager::BackupManager(IBackupStrategy* strategy) : m_strategy(strategy) {
    if (!m_strategy) {
        throw std::invalid_argument("Backup strategy cannot be null.");
    }
}

BackupManager::~BackupManager() = default;

bool BackupManager::performBackup(const std::string& source, const std::string& dest) {
    if (source.empty() || dest.empty()) {
        std::cerr << "Error: Source or destination path is empty.\n";
        return false;
    }
    return m_strategy->backup(source, dest);
}

bool BackupManager::performRestore(const std::string& backupPath, const std::string& target) {
    if (backupPath.empty() || target.empty()) {
        std::cerr << "Error: Backup path or target path is empty.\n";
        return false;
    }
    return m_strategy->restore(backupPath, target);
}