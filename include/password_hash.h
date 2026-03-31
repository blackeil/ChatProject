#pragma once

#include <string>

namespace password_hash {
    // 对明文密码做 bcrypt 哈希，cost 建议 10 - 12
    std::string hash(const std::string& password, int cost = 12);

    //检验明文密码是否与哈希匹配
    bool verify(const std::string& password, const std::string& hash);
}