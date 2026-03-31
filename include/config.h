#pragma once
// 让配置类能正确编译，先不在业务里用
#include <string>

class Config {
public:
    bool load(const std::string& path);

    const std::string& server_host() const { return server_host_; }
    int server_port() const { return server_port_; }

    const std::string& mysql_host() const { return mysql_host_; }
    int mysql_port() const { return mysql_port_; }
    int mysql_pool_size() const { return mysql_pool_size_; }
    const std::string& mysql_database() const { return mysql_database_; }
    const std::string& mysql_user() const { return mysql_user_; }
    const std::string& mysql_password() const { return mysql_password_; }

    const std::string& redis_host() const {return redis_host_;}
    int redis_port() const {return redis_port_;}

    int token_ttl_seconds() const {return token_ttl_seconds_;}
    int bcrypt_cost() const { return bcrypt_cost_; }

private:
    std::string server_host_ = "127.0.0.1";
    int server_port_ = 8080;

    std::string mysql_host_ = "127.0.0.1";
    int mysql_port_ = 3306;
    int mysql_pool_size_ = 8;
    std::string mysql_database_ = "safe_login";
    std::string mysql_user_;
    std::string mysql_password_;

    std::string redis_host_ = "127.0.0.1";
    int redis_port_ = 6379;

    int token_ttl_seconds_ = 900;
    int bcrypt_cost_ = 12;
};
