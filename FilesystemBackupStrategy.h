#pragma once
#include "IBackupStrategy.h"
#include <string>

class FilesystemBackupStrategy : public IBackupStrategy {
public:
    bool backup(const std::string& source, const std::string& dest) override;
    bool restore(const std::string& backupPath, const std::string& target) override;

private:
    bool copyRecursively(const std::string& src, const std::string& dst);
};