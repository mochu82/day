#pragma once
#include <windows.h>        // 必须包含：定义 FILETIME 和 DWORD
#include <string>
#include <iostream>

// 完整定义 FileMetadata 结构体
struct FileMetadata {
    std::string filename;
    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    DWORD fileSize;
    DWORD fileAttributes;

    void print() const;  // 成员函数声明
};

// ✅ 全局函数声明（关键修复：在头文件中声明所有操作函数）
FileMetadata GetFileMetadata(const std::string& filePath);
bool SaveMetadataToFile(const std::string& backupFilePath, const FileMetadata& meta);
bool LoadMetadataFromFile(const std::string& backupFilePath, FileMetadata& meta);
bool SetFileMetadata(const std::string& filePath, const FileMetadata& meta);