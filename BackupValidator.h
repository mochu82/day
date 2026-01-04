#pragma once
#pragma once
#ifndef BACKUP_VALIDATOR_H
#define BACKUP_VALIDATOR_H

#include <string>

class BackupValidator {
public:
    // 比较两个目录是否内容完全一致
    static bool validate(const std::string& sourceDir, const std::string& backupDir);
};

#endif // BACKUP_VALIDATOR_H