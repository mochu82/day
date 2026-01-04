#pragma once

#ifndef DECRYPT_CHACHA20_H
#define DECRYPT_CHACHA20_H

#include <vector>
#include <cstdint>
#include <string>

class ChaCha20Poly1305_Decryptor {
public:
    /**
     * @brief 解密文件
     * @param input_path 加密文件路径
     * @param key 解密密钥（32字节）
     * @param output_path 输出文件路径
     * @return 解密后的数据
     * @throws std::runtime_error 解密失败时抛出异常
     */
    static std::vector<uint8_t> decrypt_file(
        const std::string& input_path,
        const std::vector<uint8_t>& key,
        const std::string& output_path = ""
    );

    /**
     * @brief 解密内存中的数据
     * @param ciphertext 完整的密文数据（包含密文+认证标签+Nonce）
     * @param key 解密密钥（32字节）
     * @return 解密后的明文数据
     * @throws std::runtime_error 解密失败时抛出异常
     */
    static std::vector<uint8_t> decrypt_data(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key
    );

private:
    /**
     * @brief 验证密钥长度
     * @param key 密钥
     * @throws std::runtime_error 密钥长度无效
     */
    static void validate_key(const std::vector<uint8_t>& key);

    /**
     * @brief 从加密数据中提取Nonce
     * @param ciphertext 完整的加密数据
     * @param nonce_size Nonce大小（默认12字节）
     * @return Nonce部分
     */
    static std::vector<uint8_t> extract_nonce(
        const std::vector<uint8_t>& ciphertext,
        size_t nonce_size = 12
    );

    /**
     * @brief 从加密数据中提取认证标签
     * @param ciphertext 完整的加密数据
     * @param tag_size 认证标签大小（默认16字节）
     * @return 认证标签
     */
    static std::vector<uint8_t> extract_tag(
        const std::vector<uint8_t>& ciphertext,
        size_t tag_size = 16
    );

    /**
     * @brief 从加密数据中提取密文
     * @param ciphertext 完整的加密数据
     * @param nonce_size Nonce大小
     * @param tag_size 认证标签大小
     * @return 实际密文部分
     */
    static std::vector<uint8_t> extract_actual_ciphertext(
        const std::vector<uint8_t>& ciphertext,
        size_t nonce_size = 12,
        size_t tag_size = 16
    );

    /**
     * @brief 生成解密后的文件名
     * @param encrypted_path 加密文件路径
     * @return 解密文件路径
     */
    static std::string generate_decrypted_filename(const std::string& encrypted_path);
};

#endif // DECRYPT_CHACHA20_H