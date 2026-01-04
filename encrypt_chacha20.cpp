#include "encrypt_chacha20.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
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

// 验证密钥长度
void ChaCha20Poly1305_Encryptor::validate_key(const std::vector<uint8_t>& key) {
    if (key.size() != 32) {
        throw std::runtime_error("Invalid key size for ChaCha20-Poly1305. Must be 32 bytes (256 bits)");
    }
}

// 验证Nonce长度
void ChaCha20Poly1305_Encryptor::validate_nonce(const std::vector<uint8_t>& nonce) {
    if (nonce.size() != 12) {
        throw std::runtime_error("Invalid nonce size for ChaCha20-Poly1305. Must be 12 bytes");
    }
}

// 生成随机Nonce
std::vector<uint8_t> ChaCha20Poly1305_Encryptor::generate_nonce(size_t nonce_size) {
    if (nonce_size != 12) {
        throw std::runtime_error("ChaCha20-Poly1305 requires 12-byte nonce");
    }

    std::vector<uint8_t> nonce(nonce_size);
    if (RAND_bytes(nonce.data(), static_cast<int>(nonce_size)) != 1) {
        throw std::runtime_error("Failed to generate random nonce: " + get_openssl_error());
    }
    return nonce;
}

// 生成加密文件名
std::string ChaCha20Poly1305_Encryptor::generate_encrypted_filename(const std::string& original_path) {
    size_t last_dot = original_path.find_last_of('.');
    if (last_dot == std::string::npos) {
        return original_path + ".chacha20";
    }

    std::string path_part = original_path.substr(0, last_dot);
    std::string ext_part = original_path.substr(last_dot);

    // 如果是.aes加密文件，替换为.chacha20
    if (ext_part == ".aes" || ext_part == ".encrypted") {
        return path_part + ".chacha20";
    }

    return path_part + ".chacha20";
}

// 加密内存中数据
std::vector<uint8_t> ChaCha20Poly1305_Encryptor::encrypt_data(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& nonce
) {
    // 验证密钥长度
    validate_key(key);

    // 生成或使用提供的Nonce
    std::vector<uint8_t> actual_nonce = nonce.empty() ? generate_nonce() : nonce;
    validate_nonce(actual_nonce);

    // 创建EVP上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context: " + get_openssl_error());
    }

    // 初始化加密操作 
    if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize ChaCha20-Poly1305 encryption: " + get_openssl_error());
    }

    // 设置Nonce长度
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, static_cast<int>(actual_nonce.size()), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set nonce length: " + get_openssl_error());
    }

    // 设置密钥和Nonce
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), actual_nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key and nonce: " + get_openssl_error());
    }

    // 分配密文缓冲区（密文长度等于明文长度 + 认证标签）
    std::vector<uint8_t> ciphertext(plaintext.size());
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
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get authentication tag: " + get_openssl_error());
    }

    EVP_CIPHER_CTX_free(ctx);

    // 组合最终结果：Nonce + 密文 + 认证标签
    std::vector<uint8_t> result;
    result.reserve(actual_nonce.size() + ciphertext_len + tag.size());

    // 添加Nonce
    result.insert(result.end(), actual_nonce.begin(), actual_nonce.end());
    // 添加密文
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);
    // 添加认证标签
    result.insert(result.end(), tag.begin(), tag.end());

    return result;
}

// 加密文件
std::vector<uint8_t> ChaCha20Poly1305_Encryptor::encrypt_file(
    const std::string& input_path,
    const std::vector<uint8_t>& key,
    const std::string& output_path
) {
    // 验证密钥长度
    validate_key(key);

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
        generate_encrypted_filename(input_path) : output_path;

    // 写入加密文件
    std::ofstream out_file(actual_output_path, std::ios::binary);
    if (!out_file) {
        throw std::runtime_error("Failed to create output file: " + actual_output_path);
    }

    out_file.write(reinterpret_cast<const char*>(encrypted_data.data()), encrypted_data.size());
    out_file.close();

    std::cout << "File encrypted successfully with ChaCha20-Poly1305: " << input_path
        << " -> " << actual_output_path << std::endl;
    std::cout << "Original size: " << file_size << " bytes" << std::endl;
    std::cout << "Encrypted size: " << encrypted_data.size() << " bytes" << std::endl;
    std::cout << "Algorithm: ChaCha20-Poly1305" << std::endl;

    return encrypted_data;
}