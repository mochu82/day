#pragma once
#include <string>

class ZipCompressor {
public:
    /**
     * @brief 将目标压缩为ZIP文件
     * @param sourceDir 源目录 "C:\\data"
     * @param zipPath 目标ZIP文件路径，例如"backup.zip"
     * @return 是否成功
     */
    static bool compressDirectory(const std::string& sourceDir, const std::string& zipPath);
};
