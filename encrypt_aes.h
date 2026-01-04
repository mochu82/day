#pragma once

#ifndef ENCRYPT_AES_H
#define ENCRYPT_AES_H

#include <vector>
#include <cstdint>
#include <string>

class AESGCM_Encryptor {
public:
    /**
     * @brief 从文件读取数据并进行AES-GCM加密
     * @param input_path 输入文件路径
     * @param key 加密密钥（16/24/32字节）
     * @param output_path 输出文件路径
     * @return 加密成功后，加密后的数据
     * @throws std::runtime_error 加密失败时抛出异常
     */
    static std::vector<uint8_t> encrypt_file(
        const std::string& input_path,
        const std::vector<uint8_t>& key,
        const std::string& output_path = ""
    );

    /**
     * @brief 对内存中的数据进行AES-GCM加密
     * @param plaintext 明文数据
     * @param key 加密密钥（16/24/32字节）
     * @param iv 初始化向量（默认12字节）
     * @return 加密后的数据（包含密文+认证标签+IV）
     * @throws std::runtime_error 加密失败时抛出异常 
     */
    static std::vector<uint8_t> encrypt_data(
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv = {}
    );

private:
    /**
     * @brief 生成随机初始化向量
     * @param iv_size IV大小（默认12字节）
     * @return 随机IV
     */
    static std::vector<uint8_t> generate_iv(size_t iv_size = 12);

    /**
     * @brief 生成默认备份文件名
     * @param original_path 原始文件路径
     * @return 备份文件路径
     */
    static std::string generate_backup_filename(const std::string& original_path);
};

#endif // ENCRYPT_AES_H