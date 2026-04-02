#include "config/config.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool Config::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cout << "Failed" << std::endl;
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (...) {
        return false;
    }

    if (j.contains("server")) {
        auto &s = j["server"];
        if (s.contains("host")) server_host_ = s["host"].get<std::string>();
        if (s.contains("port")) server_port_ = s["port"].get<int>();
    }

    if (j.contains("mysql")) {
        auto &m = j["mysql"];
        if (m.contains("host")) mysql_host_ = m["host"].get<std::string>();
        if (m.contains("port")) mysql_port_ = m["port"].get<int>();
        if (m.contains("pool_size")) mysql_pool_size_ = m["pool_size"].get<int>();
        if (m.contains("database")) mysql_database_ = m["database"].get<std::string>();
        if (m.contains("user")) mysql_user_ = m["user"].get<std::string>();
        if (m.contains("password")) mysql_password_ = m["password"].get<std::string>();
    }

    if(j.contains("redis")) {
        auto& r = j["redis"];
        if (r.contains("host")) redis_host_ = r["host"].get<std::string>();
        if (r.contains("port")) redis_port_ = r["port"].get<int>();
    }

    if (j.contains("auth")) {
        auto& a = j["auth"];
        if (a.contains("bcrypt_cost")) {
            bcrypt_cost_ = a["bcrypt_cost"].get<int>();
        }
        if (a.contains("token_ttl_seconds")) {
            token_ttl_seconds_ = a["token_ttl_seconds"].get<int>();
        }
    }

    return true;
}
