// PathUtils.cpp
#include "PathUtils.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cctype>
#include <algorithm>


namespace fs = std::filesystem;

// 私有辅助：去除首尾双引号
static std::string TrimQuotes(const std::string& str) {
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

static bool IsValidDirectory(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}



// 辅助函数：去除字符串首尾空白（包括 \r, \n, 空格等）
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::string PathUtils::getUserInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return trim(input); // ? 自动清理空白和换行
}
// 交互式获取目标目录（类静态实现）
std::string PathUtils::GetTargetPath() {
    std::string input;
    while (true) {
        std::cout << "请输入目标目录路径: ";
        std::getline(std::cin, input);
        std::string cleanPath = TrimQuotes(input);

        if (!cleanPath.empty()) {
            return cleanPath;
        }
        else {
            std::cerr << "路径不能为空，请重新输入。\n";
        }
    }
}


std::string PathUtils::getValidSourcePath() {
    std::string path;
    std::cout << "请输入有效的源路径: ";
    std::getline(std::cin, path);
    // 可在此处添加路径有效性检查，如：
    // while (path.empty() || !IsValidDirectory(path)) { ... }
    return TrimQuotes(path);
}

std::string PathUtils::getValidBackupPath()
{
    std::string path;
    while (true)
    {
        std::cout << "请输入备份路径: ";
        std::getline(std::cin, path);
        if (!path.empty())
        {
            return TrimQuotes(path);
        }
        std::cout << "路径不能为空，请重新输入。\n";
    }
}