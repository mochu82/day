// crypto.h
#pragma once

#include <string>
#include <vector>
#include <memory>

/**
 * @brief 加密相关功能模块
 * 提供密钥派生、文件加密解密、密码输入的功能
 */

 // 派生加密密钥
std::vector<uint8_t> deriveEncryptionKey(const std::string& password, const std::string& algorithm);

// 加密 ZIP 文件
bool encryptZipFile(const std::string& zipPath, const std::string& password,
    const std::string& algorithm, std::string& encryptedPath);

// 解密文件
bool decryptFile(const std::string& encryptedPath, const std::string& password,
    const std::string& algorithm, std::string& decryptedPath);

// 选择加密算法
std::string selectEncryptionAlgorithm();

// 获取密码输入
std::string getPasswordInput(const std::string& prompt, bool confirm = false);