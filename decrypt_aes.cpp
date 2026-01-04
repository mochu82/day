#include "decrypt_aes.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

// 输出OpenSSL错误信息
static std::string get_openssl_error() {
    char error_buf[256];
    ERR_error_string_n(ERR_get_error(), error_buf, sizeof(error_buf));
    return std::string(error_buf);
}

// 提取IV
std::vector<uint8_t> AESGCM_Decryptor::extract_iv(
    const std::vector<uint8_t>& ciphertext,
    size_t iv_size
) {
    if (ciphertext.size() < iv_size) {
        throw std::runtime_error("Ciphertext too short to contain IV");
    }
    return std::vector<uint8_t>(ciphertext.begin(), ciphertext.begin() + iv_size);
}

// 提取认证标签
std::vector<uint8_t> AESGCM_Decryptor::extract_tag(
    const std::vector<uint8_t>& ciphertext,
    size_t tag_size
) {
    if (ciphertext.size() < tag_size) {
        throw std::runtime_error("Ciphertext too short to contain tag");
    }
    return std::vector<uint8_t>(ciphertext.end() - tag_size, ciphertext.end());
}

// 提取实际密文
std::vector<uint8_t> AESGCM_Decryptor::extract_actual_ciphertext(
    const std::vector<uint8_t>& ciphertext,
    size_t iv_size,
    size_t tag_size
) {
    if (ciphertext.size() < iv_size + tag_size) {
        throw std::runtime_error("Ciphertext too short");
    }
    return std::vector<uint8_t>(
        ciphertext.begin() + iv_size,
        ciphertext.end() - tag_size
    );
}

// 生成解密文件名
std::string AESGCM_Decryptor::generate_decrypted_filename(const std::string& encrypted_path) {
    size_t last_dot = encrypted_path.find_last_of('.');
    if (last_dot == std::string::npos) {
        return encrypted_path + ".decrypted";
    }

    std::string ext = encrypted_path.substr(last_dot);
    if (ext == ".backup") {
        return encrypted_path.substr(0, last_dot) + ".decrypted";
    }
    return encrypted_path + ".decrypted";
}

// 解密内存中数据
std::vector<uint8_t> AESGCM_Decryptor::decrypt_data(
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& key
) {
    // 验证密钥长度
    if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
        throw std::runtime_error("Invalid key size. Must be 16, 24, or 32 bytes");
    }

    // 提取iv和tag
    size_t iv_size = 12;
    size_t tag_size = 16;

    std::vector<uint8_t> iv = extract_iv(ciphertext, iv_size);
    std::vector<uint8_t> tag = extract_tag(ciphertext, tag_size);
    std::vector<uint8_t> actual_ciphertext = extract_actual_ciphertext(ciphertext, iv_size, tag_size);

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

    // 初始化解密操作
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption: " + get_openssl_error());
    }

    // 设置IV长度
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv_size), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set IV length: " + get_openssl_error());
    }

    // 设置密钥和IV
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key and IV: " + get_openssl_error());
    }

    // 分配明文缓冲区
    std::vector<uint8_t> plaintext(actual_ciphertext.size() + EVP_CIPHER_CTX_block_size(ctx));
    int len = 0;
    int plaintext_len = 0;

    // 解密数据
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
        actual_ciphertext.data(),
        static_cast<int>(actual_ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt data: " + get_openssl_error());
    }
    plaintext_len = len;

    // 设置认证标签
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag_size), tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set authentication tag: " + get_openssl_error());
    }

    // 完成解密并验证标签
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption failed: authentication tag mismatch");
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // 调整大小为实际明文大小
    plaintext.resize(plaintext_len);
    return plaintext;
}

// 解密文件
std::vector<uint8_t> AESGCM_Decryptor::decrypt_file(
    const std::string& input_path,
    const std::vector<uint8_t>& key,
    const std::string& output_path
) {
    // 读取加密文件
    std::ifstream file(input_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open encrypted file: " + input_path);
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 读取加密数据
    std::vector<uint8_t> encrypted_data(file_size);
    if (!file.read(reinterpret_cast<char*>(encrypted_data.data()), file_size)) {
        throw std::runtime_error("Failed to read encrypted file: " + input_path);
    }
    file.close();

    // 解密数据
    std::vector<uint8_t> decrypted_data = decrypt_data(encrypted_data, key);

    // 确认输出路径
    std::string actual_output_path = output_path.empty() ?
        generate_decrypted_filename(input_path) : output_path;

    // 写入解密文件
    std::ofstream out_file(actual_output_path, std::ios::binary);
    if (!out_file) {
        throw std::runtime_error("Failed to create output file: " + actual_output_path);
    }

    out_file.write(reinterpret_cast<const char*>(decrypted_data.data()), decrypted_data.size());
    out_file.close();

    std::cout << "File decrypted successfully: " << input_path
        << " -> " << actual_output_path << std::endl;
    std::cout << "Encrypted size: " << file_size << " bytes" << std::endl;
    std::cout << "Decrypted size: " << decrypted_data.size() << " bytes" << std::endl;

    return decrypted_data;
}