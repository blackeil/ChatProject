#pragma once

#include <string>
#include <optional>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <vector>

// 前置声明，避免在头文件直接 include <mysql/mysql.h>
struct MYSQL;

struct UserRecord {
    unsigned int id;
    std::string username;
    std::string password_hash;
};

class MysqlUserRepo {
public:
    MysqlUserRepo(const std::string& host, int port, const std::string& db,
                  const std::string& user, const std::string& password,
                  int pool_size = 8);
    ~MysqlUserRepo();

    bool connect();
    bool is_connected() const;

    // 注册：插入用户，返回用户 id；username 已存在返回 0
    unsigned int create_user(const std::string& username, const std::string& password_hash);

    // 登录：按 username 查询，不存在返回 nullopt
    std::optional<UserRecord> get_by_username(const std::string& username);
    std::optional<UserRecord> get_by_id(unsigned int user_id);
    bool update_password_hash(unsigned int user_id, const std::string& new_password_hash);
    //std::optional<T> 要么包含一个类型为 T 的值，要么不包含任何值（称为“空状态”）
    // has_value 判断是否有值

    std::vector<std::string> list_all_usernames();  // SELECT username FROM users;
private:
    // 连接池：pool_ 持有所有连接，idle_ 管理空闲连接索引。
    std::vector<MYSQL*> pool_;
    std::queue<MYSQL*> idle_;
    mutable std::mutex pool_mu_;
    std::condition_variable pool_cv_;
    bool connected_ = false;

    std::string host_;
    int port_;
    std::string db_;
    std::string user_;
    std::string password_;
    std::size_t pool_size_;

    MYSQL* acquire_connection();
    void release_connection(MYSQL* conn);
    void close_all_connections();    
};
