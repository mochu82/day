#pragma once
#include <string>

class ZipExtractor {
public:
    /**
     * @brief 将 ZIP 文件解压到目标目录
     * @param zipPath ZIP 文件路径
     * @param destDir 目标目录
     * @return 是否成功
     */
    static bool extractToDirectory(const std::string& zipPath, const std::string& destDir);
};