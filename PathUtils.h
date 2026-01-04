#pragma once
#include <string>

class PathUtils {
public:
    static std::string getValidSourcePath();
    static std::string getValidBackupPath();
    static std::string getUserInput(const std::string& prompt);
    static std::string GetTargetPath();
};