#pragma once
#include "IBackupStrategy.h"
#include <string>

class BackupManager {
public:
    explicit BackupManager(IBackupStrategy* strategy);
    ~BackupManager();

    bool performBackup(const std::string& source, const std::string& dest);
    bool performRestore(const std::string& backupPath, const std::string& target);

private:
    IBackupStrategy* m_strategy;
};