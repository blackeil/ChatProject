#include "../include/password_hash.h"

#include <crypt.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <chrono>

namespace password_hash {
    std::string hash(const std::string& password, int cost) {
        char salt[64];
        // 生成指定方法的随机盐
        char* s = crypt_gensalt("$2b$", cost, nullptr, 0);
        if (!s) return {};
        // 用于‌安全复制 C 风格字符串‌，目标、源字符串、最多复制字符
        std::strncpy(salt, s, sizeof(salt) - 1);
        salt[sizeof(salt) - 1] = '\0';

        crypt_data data{};
        data.initialized = 0;
        // 对输入字符串 key 使用指定盐值 salt 进行哈希加密
        // 返回指向线程专属缓冲区的指针
        char* h = crypt_r(password.c_str(), salt, &data);
        if (!h) return {};
        return std::string(h);
    }
    bool verify(const std::string& password, const std::string& hash_str) {
        crypt_data data{};
        data.initialized = 0;
        auto start = std::chrono::high_resolution_clock::now();
        // 这里的 crypt 会从“存的 hash”里解析出 salt 和参数，用同样的算法再算一遍
        char* result = crypt_r(password.c_str(), hash_str.c_str(), &data);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "crypt_r() 函数执行时间：" << duration.count() << " 微妙" << std::endl;
        if (!result) return false;
        //  C 语言中用于比较两个字符串的函数，第一个字符串指针、第二个字符串指针
        return std::strcmp(result, hash_str.c_str()) == 0;
    }
}

/*
相同密码之所以会得到不同的存储结果，是因为每次使用了不同的随机盐；
crypt 校验时会从数据库中那条 hash 字符串里解析出原来的算法和盐，再对输入密码重新计算并比较。
*/

/*
crypt() 会返回一个指向静态数据的指针，因此不是线程安全的；如果要在线程环境里用，应当改用 crypt_r()
crypt_r() 返回一个指向我传入的 crypt_data 缓冲区

具体作用：
根据密码明文
加上已有 hash 里的盐和算法信息
重新计算一次 hash
再和数据库里存的 hash 比较
*/
    