#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <mutex>
#include <vector>

class UsernameBloomFilter {
public:
    // expected_items: 预计用户名数量
    // false_positive_rate: 目标误判率，例如 0.001
    UsernameBloomFilter(std::size_t expected_items, double false_positive_rate)
        : expected_items_(std::max<std::size_t>(expected_items, 1)),
          false_positive_rate_(normalize_false_positive_rate(false_positive_rate)),
          bit_count_(calcBitCount(expected_items_, false_positive_rate_)),
          hash_count_(calcHashCount(bit_count_, expected_items_)),
          bits_((bit_count_ + 63) / 64, 0) {}

    // 用全量用户名重建，适合服务启动时调用
    void rebuild(const std::vector<std::string>& usernames) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        clearAllBits();
        for (const auto& username : usernames) {
            addUnlocked(username);
        }
    }

    // 注册成功后增量加入
    void add(std::string_view username) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        addUnlocked(username);
    }

    // 登录前判断：检查这 k 个位置是否都为 1，只要有一个为 0，就一定不存在
    bool possiblyContains(std::string_view username) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return possiblyContainsUnlocked(username);
    }

    // 一些只读信息，方便调试
    std::size_t bitCount() const noexcept { return bit_count_; }
    std::size_t hashCount() const noexcept { return hash_count_; }
    std::size_t byteSize() const noexcept { return bits_.size() * sizeof(std::uint64_t); }
    std::size_t expectedItems() const noexcept { return expected_items_; }
    double falsePositiveRate() const noexcept { return false_positive_rate_; }

private:
    static constexpr double kDefaultFalsePositiveRate = 0.001;
    static constexpr std::size_t kMinBitCount = 64;

    // 参数
    std::size_t expected_items_;  // 预计用户量，构造时确定容量
    double false_positive_rate_;  // 目标误判率，构造时确定
    std::size_t bit_count_;       // m
    std::size_t hash_count_;      // k
    std::size_t inserted_count_;

    // 位数组，自己做 bit 操作
    std::vector<std::uint64_t> bits_;

    // 并发控制：登录大量读，注册少量写
    mutable std::shared_mutex mutex_;  // 登录是高频读，注册是低频写，读写锁正合适

private:
    static double normalize_false_positive_rate(double p) {
        if (!(p > 0.0 && p < 1.0)) return kDefaultFalsePositiveRate;
        return p;
    }

    static std::size_t calcBitCount(std::size_t n, double p) {
        const double nn = static_cast<double>(std::max<std::size_t>(n, 1));
        const double mm = -1.0 * nn * std::log(p) / (std::log(2.0) * std::log(2.0));
        const std::size_t m = static_cast<std::size_t>(std::ceil(mm));    // 向上取整操作，返回大于或等于输入值的最小整数
        return std::max<std::size_t>(m, kMinBitCount);
    }
    // 位数组大小：m = −nlnp​/(ln2)^2

    static std::size_t calcHashCount(std::size_t m, std::size_t n) {
        const double nn = static_cast<double>(std::max<std::size_t>(n, 1));
        const double kk = (static_cast<double>(m) / nn) * std::log(2.0);
        const std::size_t k = static_cast<std::size_t>(std::ceil(kk));
        return std::max<std::size_t>(k, 1);
    }
    // 哈希个数：k = m/nln2

    // FNV-1a 64-bit
    static std::uint64_t hash1(std::string_view s) {
        std::uint64_t h = 1469598103934665603ULL;
        for (unsigned char c : s) {
            h ^= static_cast<std::uint64_t>(c);
            h *= 1099511628211ULL;
        }
        return h;
    }

    // 先算 hash1，再做一轮 bit mixing
    static std::uint64_t hash2(std::string_view s) {
        std::uint64_t x = hash1(s) ^ 0x9e3779b97f4a7c15ULL;
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x | 1ULL;
    }

    // double hashing
    std::size_t nthHash(std::uint64_t h1, std::uint64_t h2, std::size_t i) const {
        const std::uint64_t combined = h1 + static_cast<std::uint64_t>(i) * h2;
        return static_cast<std::size_t>(combined % static_cast<std::uint64_t>(bit_count_));
    }

    void setBit(std::size_t index) {
        bits_[index / 64] |= (1ULL << (index % 64));
    }

    bool testBit(std::size_t index) const {
        return (bits_[index / 64] & (1ULL << (index % 64))) != 0;
    }

    void clearAllBits() {
        std::fill(bits_.begin(), bits_.end(), 0ULL);
    }

    void addUnlocked(std::string_view username) {
        if (username.empty()) return;
        const std::uint64_t h1 = hash1(username);
        const std::uint64_t h2 = hash2(username);
        for (std::size_t i = 0; i < hash_count_; ++i) {
            setBit(nthHash(h1, h2, i));
        }
    }

    bool possiblyContainsUnlocked(std::string_view username) const {
        if (username.empty()) return false;
        const std::uint64_t h1 = hash1(username);
        const std::uint64_t h2 = hash2(username);
        for (std::size_t i = 0; i < hash_count_; ++i) {
            if (!testBit(nthHash(h1, h2, i))) return false;
        }
        return true;
    }
};


/*
容量退化策略：（待优化）
要么定期 rebuild
要么用户规模上来后重新创建更大的 BF
要么你自己维护 inserted_count_，超过阈值就报警/重建
*/ 