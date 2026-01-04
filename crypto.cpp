// crypto.cpp
#include "crypto.h"
#include "encrypt_aes.h"
#include "encrypt_chacha20.h"
#include "decrypt_aes.h"
#include "decrypt_chacha20.h"
#include "HKDF_HMAC_SHA256.h"
#include "PathUtils.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>


// 将 std::string 转换为 std::vector<uint8_t>
static std::vector<uint8_t> stringToBytes(const std::string& str)
{
    return std::vector<uint8_t>(str.begin(), str.end());
}

// 派生加密密钥
std::vector<uint8_t> deriveEncryptionKey(const std::string& password, const std::string& algorithm) {
    // 固定盐值，若场景对安全性要求更高可使用随机盐
    std::vector<uint8_t> salt = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
    };

    // 上下文信息，区分不同算法的密钥
    std::string info_str = algorithm + "-backup-key";
    std::vector<uint8_t> info = stringToBytes(info_str);

    // 输入密钥材料
    std::vector<uint8_t> ikm = stringToBytes(password);

    // 密钥长度：AES-256和ChaCha20都需要32字节
    size_t key_length = 32;

    // 使用 HKDF 派生密钥
    return HKDF_HMAC_SHA256::derive(salt, ikm, info, key_length);
}

// 加密 ZIP 文件
bool encryptZipFile(const std::string& zipPath, const std::string& password,
    const std::string& algorithm, std::string& encryptedPath) {
    try {
        // 派生密钥
        std::vector<uint8_t> key = deriveEncryptionKey(password, algorithm);

        std::cout << "正在使用" << algorithm << "算法加密ZIP文件..." << std::endl;

        // 根据选择的算法进行加密
        std::vector<uint8_t> encryptedData;
        if (algorithm == "AES-GCM") {
            encryptedData = AESGCM_Encryptor::encrypt_file(zipPath, key, zipPath + ".enc");
            encryptedPath = zipPath + ".enc";
        }
        else if (algorithm == "ChaCha20-Poly1305") {
            encryptedData = ChaCha20Poly1305_Encryptor::encrypt_file(zipPath, key, zipPath + ".cha");
            encryptedPath = zipPath + ".cha";
        }
        else {
            std::cerr << "不支持的加密算法: " << algorithm << std::endl;
            return false;
        }

        std::cout << "加密完成: " << encryptedPath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "加密失败: " << e.what() << std::endl;
        return false;
    }
}

// 解密文件
bool decryptFile(const std::string& encryptedPath, const std::string& password,
    const std::string& algorithm, std::string& decryptedPath) {
    try {
        // 产生派生的密钥
        std::vector<uint8_t> key = deriveEncryptionKey(password, algorithm);

        std::cout << "正在使用" << algorithm << "算法解密文件..." << std::endl;

        // 根据加密算法选择解密方法
        std::vector<uint8_t> decryptedData;
        if (algorithm == "AES-GCM") {
            decryptedPath = encryptedPath.substr(0, encryptedPath.find_last_of('.'));
            decryptedData = AESGCM_Decryptor::decrypt_file(encryptedPath, key, decryptedPath);
        }
        else if (algorithm == "ChaCha20-Poly1305") {
            decryptedPath = encryptedPath.substr(0, encryptedPath.find_last_of('.'));
            decryptedData = ChaCha20Poly1305_Decryptor::decrypt_file(encryptedPath, key, decryptedPath);
        }
        else {
            std::cerr << "不支持的加密算法: " << algorithm << std::endl;
            return false;
        }

        std::cout << "解密完成: " << decryptedPath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "解密失败: " << e.what() << std::endl;
        return false;
    }
}

// 选择加密算法
std::string selectEncryptionAlgorithm() {
    std::cout << "\n请选择加密算法:" << std::endl;
    std::cout << "1. AES-GCM " << std::endl;
    std::cout << "2. ChaCha20-Poly1305" << std::endl;
    std::cout << "请输入选择 (1-2): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        return "AES-GCM";
    }
    else if (choice == "2") {
        return "ChaCha20-Poly1305";
    }
    else {
        std::cout << "无效选择，使用默认算法AES-GCM" << std::endl;
        return "AES-GCM";
    }
}

// 获取密码输入
std::string getPasswordInput(const std::string& prompt, bool confirm) {
    std::string password, confirmPassword;

    // 输入密码可尝试3次
    for (int attempts = 1; attempts <= 3; attempts++) {
        std::cout << prompt << "(已尝试 " << attempts << "/3): ";
        std::getline(std::cin, password);

        if (password.empty()) {
            std::cout << "密码不能为空。" << std::endl;
            if (attempts == 3) return "";
            continue;
        }

        if (confirm) {
            std::cout << "请再次输入密码确认: ";
            std::getline(std::cin, confirmPassword);

            if (password != confirmPassword) {
                std::cout << "密码不匹配！" << std::endl;
                if (attempts == 3) {
                    std::cout << "超过最大尝试次数，操作已取消。" << std::endl;
                    return "";
                }
                continue;
            }
        }

        return password; // 密码有效
    }

    return ""; // 超过尝试次数
}