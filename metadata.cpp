#include "metadata.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ✅ 辅助函数：FILETIME 转宽字符串（修复 E0020: FILETIME 未定义）
std::wstring FileTimeToString(FILETIME ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    wchar_t buffer[100];
    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buffer);
}

// ✅ 成员函数实现（修复 VCR001: print() 未定义）
void FileMetadata::print() const {
    std::wcout << L"文件名: " << std::wstring(filename.begin(), filename.end()) << L"\n";
    std::wcout << L"创建时间: " << FileTimeToString(creationTime) << L"\n";
    std::wcout << L"最后访问: " << FileTimeToString(lastAccessTime) << L"\n";
    std::wcout << L"最后修改: " << FileTimeToString(lastWriteTime) << L"\n";
    std::wcout << L"文件大小: " << fileSize << L" 字节\n";
    std::wcout << L"文件属性: 0x" << std::hex << fileAttributes << std::dec << L"\n";
}

// ✅ 全局函数实现（修复 E0020: 未定义标识符）
FileMetadata GetFileMetadata(const std::string& filePath) {
    FileMetadata meta;
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExA(filePath.c_str(), GetFileExInfoStandard, &attr)) {
        std::cerr << "[错误] 无法获取文件属性: " << filePath << "\n";
        return meta; // 返回空对象，调用者需检查
    }

    meta.filename = fs::path(filePath).filename().string();
    meta.creationTime = attr.ftCreationTime;
    meta.lastAccessTime = attr.ftLastAccessTime;
    meta.lastWriteTime = attr.ftLastWriteTime;
    meta.fileSize = static_cast<DWORD>(attr.nFileSizeLow);
    meta.fileAttributes = attr.dwFileAttributes;
    return meta;
}

bool SaveMetadataToFile(const std::string& backupFilePath, const FileMetadata& meta) {
    json j;
    j["filename"] = meta.filename;
    j["creationTime"]["low"] = meta.creationTime.dwLowDateTime;
    j["creationTime"]["high"] = meta.creationTime.dwHighDateTime;
    j["lastAccessTime"]["low"] = meta.lastAccessTime.dwLowDateTime;
    j["lastAccessTime"]["high"] = meta.lastAccessTime.dwHighDateTime;
    j["lastWriteTime"]["low"] = meta.lastWriteTime.dwLowDateTime;
    j["lastWriteTime"]["high"] = meta.lastWriteTime.dwHighDateTime;
    j["fileSize"] = meta.fileSize;
    j["fileAttributes"] = meta.fileAttributes;

    std::ofstream ofs(backupFilePath + ".meta");
    if (!ofs.is_open()) {
        std::cerr << "[错误] 无法创建元数据文件: " << backupFilePath + ".meta\n";
        return false;
    }
    ofs << j.dump(4);
    ofs.close();
    return true;
}

bool LoadMetadataFromFile(const std::string& backupFilePath, FileMetadata& meta) {
    std::ifstream ifs(backupFilePath + ".meta");
    if (!ifs.is_open()) {
        std::cerr << "[错误] 无法打开元数据文件: " << backupFilePath + ".meta\n";
        return false;
    }

    try {
        json j;
        ifs >> j;
        meta.filename = j.at("filename").get<std::string>();
        meta.creationTime.dwLowDateTime = j.at("creationTime").at("low").get<ULONG>();
        meta.creationTime.dwHighDateTime = j.at("creationTime").at("high").get<ULONG>();
        meta.lastAccessTime.dwLowDateTime = j.at("lastAccessTime").at("low").get<ULONG>();
        meta.lastAccessTime.dwHighDateTime = j.at("lastAccessTime").at("high").get<ULONG>();
        meta.lastWriteTime.dwLowDateTime = j.at("lastWriteTime").at("low").get<ULONG>();
        meta.lastWriteTime.dwHighDateTime = j.at("lastWriteTime").at("high").get<ULONG>();
        meta.fileSize = j.at("fileSize").get<DWORD>();
        meta.fileAttributes = j.at("fileAttributes").get<DWORD>();
    }
    catch (const std::exception& e) {
        std::cerr << "[错误] 解析 JSON 失败: " << e.what() << "\n";
        return false;
    }
    return true;
}

bool SetFileMetadata(const std::string& filePath, const FileMetadata& meta) {
    HANDLE hFile = CreateFileA(
        filePath.c_str(),
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "[错误] 无法打开文件以设置元数据: " << filePath << "\n";
        return false;
    }

    bool success = true;
    if (!SetFileTime(hFile, &meta.creationTime, &meta.lastAccessTime, &meta.lastWriteTime)) {
        std::cerr << "[警告] 设置时间戳失败\n";
        success = false;
    }

    CloseHandle(hFile);

    if (!SetFileAttributesA(filePath.c_str(), meta.fileAttributes)) {
        std::cerr << "[警告] 设置文件属性失败\n";
        success = false;
    }

    return success;
}