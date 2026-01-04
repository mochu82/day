// ZipCompressor.cpp
#include "ZipCompressor.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include <direct.h>
#include <minizip/zip.h>

#ifdef _WIN32
#define SEPARATOR '\\'
#define IS_DIR_SEP(c) ((c) == '\\' || (c) == '/')
#else
#define SEPARATOR '/'
#define IS_DIR_SEP(c) ((c) == '/')
#endif

// 递归添加文件/目录到 ZIP
bool addFileToZip(zipFile zip, const std::string& root, const std::string& filePath) {
    //  安全拼接路径：避免 root 后多出一个 \

    std::string fullPath = root;
    if (!filePath.empty()) {
        fullPath += SEPARATOR + filePath;
    }

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(fullPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::cerr << "无法访问: " << fullPath << " (错误码: " << error << ")" << std::endl;
        return false;
    }
    FindClose(hFind);

    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // 添加目录项（以 / 结尾）
        std::string dirEntry = filePath;
        if (!dirEntry.empty() && dirEntry.back() != '/' && dirEntry.back() != '\\') {
            dirEntry += "/";
        }

        if (zipOpenNewFileInZip(zip, dirEntry.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
            std::cerr << "无法添加目录: " << dirEntry << std::endl;
        }
        else {
            zipCloseFileInZip(zip);  // 必须关闭
        }

        // 遍历子项
        std::string searchPath = fullPath + "\\*";
        hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            FindClose(hFind);
            return true; // 空目录也算成功
        }

        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;

            std::string childPath = filePath.empty() ? name : (filePath + "/" + name);
            if (!addFileToZip(zip, root, childPath)) {  // 递归并检查返回值
                FindClose(hFind);
                return false;
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    else {
        // 添加文件
        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << fullPath << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(fileSize);
        if (!file.read(buffer.data(), fileSize)) {
            std::cerr << "读取文件失败: " << fullPath << std::endl;
            file.close();
            return false;
        }
        file.close();

        if (zipOpenNewFileInZip(zip, filePath.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
            std::cerr << "无法添加文件: " << filePath << std::endl;
            return false;
        }

        zipWriteInFileInZip(zip, buffer.data(), static_cast<unsigned int>(fileSize));
        zipCloseFileInZip(zip);  // 必须关闭
    }
    return true;
}

bool ZipCompressor::compressDirectory(const std::string& sourceDir, const std::string& zipPath) {
    //  清理源路径：移除末尾的 \ 或 /
    std::string cleanSourceDir = sourceDir;
    while (!cleanSourceDir.empty() &&
        (cleanSourceDir.back() == '\\' || cleanSourceDir.back() == '/')) {
        cleanSourceDir.pop_back();
    }

    //  检查目录是否存在
    if (GetFileAttributesA(cleanSourceDir.c_str()) == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        std::cerr << "源目录不存在或无法访问: " << cleanSourceDir << " (错误码: " << error << ")" << std::endl;
        return false;
    }

    // 创建 ZIP 文件
    zipFile zip = zipOpen(zipPath.c_str(), APPEND_STATUS_CREATE);
    if (!zip) {
        std::cerr << "无法创建 ZIP 文件: " << zipPath << std::endl;
        return false;
    }

    // 开始压缩
    bool success = addFileToZip(zip, cleanSourceDir, "");  // 从根开始

    // 关闭 ZIP
    zipClose(zip, nullptr);

    if (success) {
        std::cout << " 压缩成功: " << zipPath << std::endl;
    }
    else {
        std::cerr << " 压缩失败，请检查源目录是否存在或有权限" << std::endl;
    }
    return success;
}