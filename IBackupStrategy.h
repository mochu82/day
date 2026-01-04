#pragma once
#include <string>

class IBackupStrategy {
public:
    virtual ~IBackupStrategy() = default;

    virtual bool backup(const std::string& source, const std::string& dest) = 0;
    virtual bool restore(const std::string& backupPath, const std::string& target) = 0;
};