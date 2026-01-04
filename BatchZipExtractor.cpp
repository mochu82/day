#include "BatchZipExtractor.h"
#include <minizip/unzip.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// 创建父目录
static void createParentDirs(const std::string& filePath) {
    fs::create_directories(fs::path(filePath).parent_path());
}

// 解压当前打开的文件到 outputPath
static bool extractCurrentFile(unzFile unzip, const std::string& outputPath) {
    std::ofstream out(outputPath, std::ios::binary);
    if (!out) return false;

    char buffer[8192];
    int err;
    while ((err = unzReadCurrentFile(unzip, buffer, sizeof(buffer))) > 0) {
        out.write(buffer, err);
    }
    out.close();
    return (err == 0); // 0 表示成功读完
}

bool BatchZipExtractor::splitExtract(const std::string& zipPath, const std::string& outputDir1, const std::string& outputDir2) {
    unzFile unzip = unzOpen(zipPath.c_str());
    if (!unzip) {
        return false;
    }

    unz_global_info globalInfo;
    if (unzGetGlobalInfo(unzip, &globalInfo) != UNZ_OK) {
        unzClose(unzip);
        return false;
    }

    // 第一遍：收集所有非目录文件路径
    std::vector<std::string> files;
    if (globalInfo.number_entry > 0 && unzGoToFirstFile(unzip) == UNZ_OK) {
        for (unsigned i = 0; i < globalInfo.number_entry; ++i) {
            char filename[512] = { 0 };
            unz_file_info fileInfo;
            if (unzGetCurrentFileInfo(unzip, &fileInfo, filename, sizeof(filename), nullptr, 0, nullptr, 0) == UNZ_OK) {
                // 跳过目录（minizip 中目录通常 size==0 且以 '/' 结尾）
                if (filename[0] != '\0' && filename[strlen(filename) - 1] != '/') {
                    files.push_back(std::string(filename));
                }
            }
            if (i + 1 < globalInfo.number_entry) {
                unzGoToNextFile(unzip);
            }
        }
    }

    unzClose(unzip);

    if (files.empty()) {
        // 没有文件也视为成功
        fs::create_directories(outputDir1);
        fs::create_directories(outputDir2);
        return true;
    }

    // 计算分割点：前 half 个给 outputDir1，其余给 outputDir2
    size_t total = files.size();
    size_t half = (total + 1) / 2; // 向上取整 → 第一份多一个（若奇数）

    // 重新打开 ZIP 进行解压
    unzip = unzOpen(zipPath.c_str());
    if (!unzip) {
        return false;
    }

    fs::create_directories(outputDir1);
    fs::create_directories(outputDir2);

    bool success = true;

    for (size_t i = 0; i < files.size(); ++i) {
        const std::string& file = files[i];
        if (unzLocateFile(unzip, file.c_str(), 0) != UNZ_OK) {
            success = false;
            continue;
        }
        if (unzOpenCurrentFile(unzip) != UNZ_OK) {
            success = false;
            continue;
        }

        std::string targetDir = (i < half) ? outputDir1 : outputDir2;
        std::string outputPath = (fs::path(targetDir) / file).string();
        createParentDirs(outputPath);

        if (!extractCurrentFile(unzip, outputPath)) {
            success = false;
        }

        unzCloseCurrentFile(unzip);
    }

    unzClose(unzip);
    return success;
}