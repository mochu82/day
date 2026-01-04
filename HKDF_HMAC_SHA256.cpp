#include "HKDF_HMAC_SHA256.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>

// HMAC-SHA256实现（使用OpenSSL的EVP_MAC API）
std::vector<uint8_t> HKDF_HMAC_SHA256::hmac_sha256(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data
) {
    std::vector<uint8_t> result(SHA256_DIGEST_LENGTH);
    size_t out_len = SHA256_DIGEST_LENGTH;

    // 调用EVP_MAC API
    EVP_MAC* mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    if (!mac) {
        throw std::runtime_error("Failed to fetch HMAC algorithm");
    }

    EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
    if (!ctx) {
        EVP_MAC_free(mac);
        throw std::runtime_error("Failed to create HMAC context");
    }

    // 设置 HMAC 密钥
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string("digest", const_cast<char*>("SHA256"), 0);
    params[1] = OSSL_PARAM_construct_end();

    if (!EVP_MAC_init(ctx, key.data(), key.size(), params)) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        throw std::runtime_error("Failed to initialize HMAC");
    }

    // 更新数据
    if (!EVP_MAC_update(ctx, data.data(), data.size())) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        throw std::runtime_error("Failed to update HMAC");
    }

    // 获取结果
    if (!EVP_MAC_final(ctx, result.data(), &out_len, result.size())) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        throw std::runtime_error("Failed to finalize HMAC");
    }

    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    return result;
}

// 连接两个字节数组
std::vector<uint8_t> HKDF_HMAC_SHA256::concat(
    const std::vector<uint8_t>& a,
    const std::vector<uint8_t>& b
) {
    std::vector<uint8_t> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// HKDF提取阶段
std::vector<uint8_t> HKDF_HMAC_SHA256::extract(
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& ikm
) {
    std::vector<uint8_t> actual_salt = salt;
    if (actual_salt.empty()) {
        actual_salt.resize(SHA256_DIGEST_LENGTH, 0);
    }
    return hmac_sha256(actual_salt, ikm);
}

// HKDF扩展阶段
std::vector<uint8_t> HKDF_HMAC_SHA256::expand(
    const std::vector<uint8_t>& prk,
    const std::vector<uint8_t>& info,
    size_t length
) {
    if (prk.size() < SHA256_DIGEST_LENGTH) {
        throw std::runtime_error("PRK is too short");
    }

    const size_t hash_len = SHA256_DIGEST_LENGTH;
    const size_t n = (length + hash_len - 1) / hash_len;

    if (n > 255) {
        throw std::runtime_error("Requested output length too large");
    }

    std::vector<uint8_t> okm;
    std::vector<uint8_t> t;
    //T(N)运算
    for (size_t i = 1; i <= n; ++i) {
        std::vector<uint8_t> input;

        if (i == 1) {
            input = concat(info, { static_cast<uint8_t>(i) });
        }
        else {
            input = t;
            input.insert(input.end(), info.begin(), info.end());
            input.push_back(static_cast<uint8_t>(i));
        }

        t = hmac_sha256(prk, input);

        okm.insert(okm.end(), t.begin(), t.end());
    }

    okm.resize(length);
    return okm;
}

// HKDF完整派生过程
std::vector<uint8_t> HKDF_HMAC_SHA256::derive(
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& ikm,
    const std::vector<uint8_t>& info,
    size_t length
) {
    // 提取阶段
    std::vector<uint8_t> prk = extract(salt, ikm);

    // 扩展阶段
    return expand(prk, info, length);
}