// ZipExtractor.cpp
#include "ZipExtractor.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include <minizip/unzip.h>

// 清理路径：移除末尾的 \ 或 /
std::string cleanPath(const std::string& path) {
    std::string cleaned = path;
    while (!cleaned.empty() &&
        (cleaned.back() == '/' || cleaned.back() == '\\')) {
        cleaned.pop_back();
    }
    return cleaned;
}

// 递归创建目录（Windows 专用）
bool createDirRecursive(const std::string& dir) {
    if (dir.empty()) return true;

    std::string cleaned = dir;
    for (char& c : cleaned) {
        if (c == '/') c = '\\';
    }

    size_t pos = 0;
    bool hasDrive = (cleaned.length() >= 2 && cleaned[1] == ':');
    if (hasDrive && cleaned.length() > 3 && cleaned[2] == '\\') {
        pos = 3; // 跳过 "D:\"
    }
    else if (cleaned.length() > 1 && cleaned[0] == '\\' && cleaned[1] == '\\') {
        // 网络路径，跳过处理
    }
    else {
        pos = 0;
    }

    while ((pos = cleaned.find('\\', pos)) != std::string::npos) {
        std::string subPath = cleaned.substr(0, pos);
        pos++;

        // 跳过根目录如 D:\ 或 C:\

        if ((subPath.length() == 2 && subPath[1] == ':') ||
            (subPath.length() == 3 && subPath[1] == ':' && subPath[2] == '\\')) {
            continue;
        }

        DWORD attr = GetFileAttributesA(subPath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            if (!CreateDirectoryA(subPath.c_str(), nullptr)) {
                DWORD error = GetLastError();
                std::cerr << "无法创建目录: " << subPath << " (错误码: " << error << ")" << std::endl;
                return false;
            }
        }
    }

    // 创建最终目录
    DWORD attr = GetFileAttributesA(cleaned.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(cleaned.c_str(), nullptr)) {
            DWORD error = GetLastError();
            std::cerr << "无法创建最终目录: " << cleaned << " (错误码: " << error << ")" << std::endl;
            return false;
        }
    }

    return true;
}


// 辅助函数：安全跳转到下一个文件
static void goto_next_file(unzFile& unzip, unsigned currentIndex, unsigned totalEntries) {
    if (currentIndex + 1 < totalEntries) {
        int err = unzGoToNextFile(unzip);
        if (err != UNZ_OK) {
            std::cerr << "警告：无法跳转到下一个文件（错误码: " << err << "）\n";
            // 不返回 false，继续尝试后续（或结束）
        }
    }
}

bool ZipExtractor::extractToDirectory(const std::string& zipPath, const std::string& destDir) {
    unzFile unzip = unzOpen(zipPath.c_str());
    if (!unzip) {
        std::cerr << "无法打开 ZIP 文件: " << zipPath << std::endl;
        return false;
    }

    unz_global_info globalInfo;
    if (unzGetGlobalInfo(unzip, &globalInfo) != UNZ_OK) {
        std::cerr << "无法获取 ZIP 信息\n";
        unzClose(unzip);
        return false;
    }

    std::string cleanDestDir = cleanPath(destDir);
    if (!createDirRecursive(cleanDestDir)) {
        std::cerr << "无法创建目标目录: " << cleanDestDir << std::endl;
        unzClose(unzip);
        return false;
    }

    // 定位到第一个文件（如果 ZIP 非空）
    if (globalInfo.number_entry > 0) {
        if (unzGoToFirstFile(unzip) != UNZ_OK) {
            std::cerr << "无法定位到 ZIP 中的第一个文件\n";
            unzClose(unzip);
            return false;
        }
    }

    for (unsigned i = 0; i < globalInfo.number_entry; ++i) {
        unz_file_info fileInfo;
        char filename[512] = { 0 };

        if (unzGetCurrentFileInfo(unzip, &fileInfo, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK) {
            std::cerr << "无法读取文件信息，跳过条目 #" << i << "\n";
            goto_next_file(unzip, i, globalInfo.number_entry);
            continue;
        }

        if (strlen(filename) == 0) {
            std::cerr << "跳过空文件名条目 #" << i << "\n";
            goto_next_file(unzip, i, globalInfo.number_entry);
            continue;
        }

        std::string fileInZip = filename;

        // 简单路径穿越防护
        if (fileInZip.find("..") != std::string::npos) {
            std::cerr << "跳过潜在危险路径: " << fileInZip << "\n";
            goto_next_file(unzip, i, globalInfo.number_entry);
            continue;
        }

        // 构造本地路径（保留 '/'，Windows 兼容）
        std::string fullPath = cleanDestDir + "/" + fileInZip;
#ifdef _WIN32
        for (char& c : fullPath) {
            if (c == '/') c = '\\';
        }
#endif

        bool isDirectory = !fileInZip.empty() && (fileInZip.back() == '/');

        if (isDirectory) {
            if (!createDirRecursive(fullPath)) {
                std::cerr << "无法创建目录: " << fullPath << "\n";
            }
        }
        else {
            // 创建父目录
            size_t lastSep = fullPath.find_last_of("/\\");
            if (lastSep != std::string::npos) {
                std::string parentDir = fullPath.substr(0, lastSep);
                if (!createDirRecursive(parentDir)) {
                    std::cerr << "无法创建父目录: " << parentDir << "，跳过文件\n";
                    goto_next_file(unzip, i, globalInfo.number_entry);
                    continue;
                }
            }

            if (unzOpenCurrentFile(unzip) != UNZ_OK) {
                std::cerr << "无法打开压缩包内文件: " << filename << "\n";
                goto_next_file(unzip, i, globalInfo.number_entry);
                continue;
            }

            std::ofstream out(fullPath, std::ios::binary);
            if (!out) {
                std::cerr << "无法创建文件: " << fullPath << "\n";
                unzCloseCurrentFile(unzip);
                goto_next_file(unzip, i, globalInfo.number_entry);
                continue;
            }

            std::vector<char> buffer(8192);
            int bytesRead;
            while ((bytesRead = unzReadCurrentFile(unzip, buffer.data(), static_cast<unsigned>(buffer.size()))) > 0) {
                out.write(buffer.data(), bytesRead);
            }
            out.close();
            unzCloseCurrentFile(unzip);
        }

        // 移动到下一个文件（如果不是最后一个）
        goto_next_file(unzip, i, globalInfo.number_entry);
    }

    unzClose(unzip);
    std::cout << "解压成功: " << cleanDestDir << std::endl;
    return true;
}
