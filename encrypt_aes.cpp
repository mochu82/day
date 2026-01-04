#include "encrypt_aes.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

// 私有辅助函数，格式化并输出OpenSSL错误信息
static std::string get_openssl_error() {
    char error_buf[256];
    ERR_error_string_n(ERR_get_error(), error_buf, sizeof(error_buf));
    return std::string(error_buf);
}

// 生成随机iv
std::vector<uint8_t> AESGCM_Encryptor::generate_iv(size_t iv_size) {
    std::vector<uint8_t> iv(iv_size);
    if (RAND_bytes(iv.data(), static_cast<int>(iv_size)) != 1) {
        throw std::runtime_error("Failed to generate random IV: " + get_openssl_error());
    }
    return iv;
}

// 生成备份文件名
std::string AESGCM_Encryptor::generate_backup_filename(const std::string& original_path) {
    // 若未指定输出路径，创建备份文件
    size_t last_dot = original_path.find_last_of('.');
    if (last_dot == std::string::npos) {
        return original_path + ".backup";
    }
    return original_path.substr(0, last_dot) + ".backup";
}

// 加密内存中数据
std::vector<uint8_t> AESGCM_Encryptor::encrypt_data(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv
) {
    // 验证密钥长度
    if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
        throw std::runtime_error("Invalid key size. Must be 16, 24, or 32 bytes");
    }

    // 生成或使用提供的iv
    std::vector<uint8_t> actual_iv = iv.empty() ? generate_iv() : iv;
    if (actual_iv.size() != 12) {
        throw std::runtime_error("IV must be 12 bytes for AES-GCM");
    }

    // 创建EVP上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context: " + get_openssl_error());
    }

    // 根据密码位数，选择算法
    const EVP_CIPHER* cipher = nullptr;
    switch (key.size()) {
    case 16: cipher = EVP_aes_128_gcm(); break;
    case 24: cipher = EVP_aes_192_gcm(); break;
    case 32: cipher = EVP_aes_256_gcm(); break;
    default: throw std::runtime_error("Unsupported key size");
    }

    // 初始化加密操作
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption: " + get_openssl_error());
    }

    // 设置IV长度
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(actual_iv.size()), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set IV length: " + get_openssl_error());
    }

    // 设置密钥和IV
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), actual_iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key and IV: " + get_openssl_error());
    }

    // 分配密文缓冲区
    std::vector<uint8_t> ciphertext(plaintext.size() + EVP_CIPHER_CTX_block_size(ctx));
    int len = 0;
    int ciphertext_len = 0;

    // 加密数据
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to encrypt data: " + get_openssl_error());
    }
    ciphertext_len = len;

    // 结束加密
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize encryption: " + get_openssl_error());
    }
    ciphertext_len += len;

    // 获取认证标签
    std::vector<uint8_t> tag(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get authentication tag: " + get_openssl_error());
    }

    EVP_CIPHER_CTX_free(ctx);

    // 组合最终结果：iv + 密文 + 认证标签
    std::vector<uint8_t> result;
    result.reserve(actual_iv.size() + ciphertext_len + tag.size());

    // 添加iv
    result.insert(result.end(), actual_iv.begin(), actual_iv.end());
    // 添加密文
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);
    // 添加认证标签
    result.insert(result.end(), tag.begin(), tag.end());

    return result;
}

// 加密文件
std::vector<uint8_t> AESGCM_Encryptor::encrypt_file(
    const std::string& input_path,
    const std::vector<uint8_t>& key,
    const std::string& output_path
) {
    // 读取文件
    std::ifstream file(input_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open input file: " + input_path);
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 读取文件内容
    std::vector<uint8_t> plaintext(file_size);
    if (!file.read(reinterpret_cast<char*>(plaintext.data()), file_size)) {
        throw std::runtime_error("Failed to read file: " + input_path);
    }
    file.close();

    // 加密数据
    std::vector<uint8_t> encrypted_data = encrypt_data(plaintext, key);

    // 确定输出路径
    std::string actual_output_path = output_path.empty() ?
        generate_backup_filename(input_path) : output_path;

    // 写入加密文件
    std::ofstream out_file(actual_output_path, std::ios::binary);
    if (!out_file) {
        throw std::runtime_error("Failed to create output file: " + actual_output_path);
    }

    out_file.write(reinterpret_cast<const char*>(encrypted_data.data()), encrypted_data.size());
    out_file.close();

    std::cout << "File encrypted successfully: " << input_path
        << " -> " << actual_output_path << std::endl;
    std::cout << "Original size: " << file_size << " bytes" << std::endl;
    std::cout << "Encrypted size: " << encrypted_data.size() << " bytes" << std::endl;

    return encrypted_data;
}