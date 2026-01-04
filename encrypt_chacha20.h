#pragma once

#ifndef ENCRYPT_CHACHA20_H
#define ENCRYPT_CHACHA20_H

#include <vector>
#include <cstdint>
#include <string>

class ChaCha20Poly1305_Encryptor {
public:
    /**
     * @brief 从文件读取数据并进行ChaCha20-Poly1305加密
     * @param input_path 输入文件路径
     * @param key 加密密钥（32字节）
     * @param output_path 输出文件路径
     * @return 加密成功时，加密后的数据
     * @throws std::runtime_error 加密失败时抛出异常
     */
    static std::vector<uint8_t> encrypt_file(
        const std::string& input_path,
        const std::vector<uint8_t>& key,
        const std::string& output_path = ""
    );

    /**
     * @brief 对内存中的数据进行ChaCha20-Poly1305加密
     * @param plaintext 明文数据
     * @param key 加密密钥（32字节）
     * @param nonce 随机数（默认12字节）
     * @return 加密后的数据（包含密文+认证标签+Nonce）
     * @throws std::runtime_error 加密失败时抛出异常
     */
    static std::vector<uint8_t> encrypt_data(
        const std::vector<uint8_t>& plaintext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce = {}
    );

    /**
     * @brief 生成随机Nonce
     * @param nonce_size Nonce大小（默认12字节）
     * @return 随机Nonce
     */
    static std::vector<uint8_t> generate_nonce(size_t nonce_size = 12);

    /**
     * @brief 生成默认加密文件名
     * @param original_path 原始文件路径
     * @return 加密文件路径
     */
    static std::string generate_encrypted_filename(const std::string& original_path);

private:
    /**
     * @brief 验证密钥长度（ChaCha20需要32字节密钥）
     * @param key 密钥
     * @throws std::runtime_error 密钥长度无效
     */
    static void validate_key(const std::vector<uint8_t>& key);

    /**
     * @brief 验证Nonce长度（推荐12字节）
     * @param nonce Nonce
     * @throws std::runtime_error Nonce长度无效
     */
    static void validate_nonce(const std::vector<uint8_t>& nonce);
};

#endif // ENCRYPT_CHACHA20_H