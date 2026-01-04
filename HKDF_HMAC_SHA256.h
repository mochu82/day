#pragma once

#ifndef HKDF_HMAC_SHA256_H
#define HKDF_HMAC_SHA256_H

#include <vector>
#include <cstdint>

class HKDF_HMAC_SHA256 {
public:
    /**
     * @brief HKDF提取阶段
     * @param salt 盐值
     * @param ikm 输入密钥材料
     * @return 提取后的伪随机密钥
     */
    static std::vector<uint8_t> extract(
        const std::vector<uint8_t>& salt,
        const std::vector<uint8_t>& ikm
    );

    /**
     * @brief HKDF扩展阶段
     * @param prk 伪随机密钥
     * @param info 上下文信息
     * @param length 期望输出长度
     * @return 派生密钥
     */
    static std::vector<uint8_t> expand(
        const std::vector<uint8_t>& prk,
        const std::vector<uint8_t>& info,
        size_t length
    );

    /**
     * @brief HKDF完整派生过程
     * @param salt 盐值
     * @param ikm 输入密钥材料
     * @param info 上下文信息
     * @param length 期望输出长度
     * @return 派生密钥
     */
    static std::vector<uint8_t> derive(
        const std::vector<uint8_t>& salt,
        const std::vector<uint8_t>& ikm,
        const std::vector<uint8_t>& info,
        size_t length
    );

private:
    /**
     * @brief HMAC-SHA256计算
     */
    static std::vector<uint8_t> hmac_sha256(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data
    );

    /**
     * @brief 连接两个字节数组
     */
    static std::vector<uint8_t> concat(
        const std::vector<uint8_t>& a,
        const std::vector<uint8_t>& b
    );
};

#endif // HKDF_HMAC_SHA256_H