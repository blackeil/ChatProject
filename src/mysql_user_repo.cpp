#include "../include/mysql_user_repo.h"

#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <vector>

namespace {

// 每次创建单个 MySQL 连接，供连接池初始化使用。
MYSQL* open_mysql_connection(const std::string& host, int port, const std::string& db,
                             const std::string& user, const std::string& password) {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) return nullptr;

    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), db.c_str(),
                            static_cast<unsigned int>(port), nullptr, 0)) {
        mysql_close(conn);
        return nullptr;
    }
    return conn;
}

// 仍沿用字符串 escape 的实现方式，仅把连接参数改为借到的连接。
std::string escape_sql(MYSQL* conn, const std::string& s) {
    if (!conn) return {};
    std::vector<char> buf(s.size() * 2 + 1);
    unsigned long out_len = mysql_real_escape_string(conn, buf.data(), s.c_str(),
                                                      static_cast<unsigned long>(s.size()));
    return std::string(buf.data(), out_len);
}

}  // namespace

MysqlUserRepo::MysqlUserRepo(const std::string& host, int port,
                             const std::string& db,
                             const std::string& user,
                             const std::string& password,
                             int pool_size)
    : host_(host),
      port_(port),
      db_(db),
      user_(user),
      password_(password),
      pool_size_(pool_size > 0 ? static_cast<std::size_t>(pool_size) : 8) {}

MysqlUserRepo::~MysqlUserRepo() {
    close_all_connections();
}

bool MysqlUserRepo::is_connected() const {
    std::lock_guard<std::mutex> lock(pool_mu_);
    return connected_;
}

bool MysqlUserRepo::connect() {
    std::lock_guard<std::mutex> lock(pool_mu_);
    if (connected_) return true;

    // 初始化连接池：一次性创建 N 条连接。
    for (std::size_t i = 0; i < pool_size_; ++i) {
        MYSQL* conn = open_mysql_connection(host_, port_, db_, user_, password_);
        if (!conn) {
            close_all_connections();
            connected_ = false;
            return false;
        }
        pool_.push_back(conn);
        idle_.push(conn);
    }
    connected_ = true;
    return true;
}

MYSQL* MysqlUserRepo::acquire_connection() {
    std::unique_lock<std::mutex> lock(pool_mu_);
    if (!connected_) return nullptr;

    pool_cv_.wait(lock, [this] { return !idle_.empty() || !connected_; });

    // 当无空闲连接时阻塞等待，避免多个线程共享同一条连接。
    pool_cv_.wait(lock, [this] { return !idle_.empty() || !connected_; });
    if (!connected_ || idle_.empty()) return nullptr;
    /*
    wait：先检查条件，条件不满足就挂起等待，被唤醒后再重新检查条件，直到条件满足才继续往下走

    条件变量挂起时会释放锁；唤醒后会重新加锁；带谓词的 wait 会自己反复检查条件，
    只有“有连接可用”或“池子关闭”时才真正返回，后面的 if 主要是做最终保险检查。
    */

    MYSQL* conn = idle_.front();
    idle_.pop();
    return conn;
}

void MysqlUserRepo::release_connection(MYSQL* conn) {
    if (!conn) return;
    {
        std::lock_guard<std::mutex> lock(pool_mu_);
        if (!connected_) return;
        idle_.push(conn);
    }
    pool_cv_.notify_one();
}

void MysqlUserRepo::close_all_connections() {
    while (!idle_.empty()) {
        idle_.pop();
    }
    for (MYSQL* conn : pool_) {
        if (conn) mysql_close(conn);
    }
    pool_.clear();
    connected_ = false;
    pool_cv_.notify_all();
    /*
    不调用 notify_all() 的话，等待线程可能永远不会被唤醒，
    因此即使 connected_ 已经变成 false，它们也可能一直阻塞在 wait 里。
    线程可能一直无法结束，程序在关闭、析构、join 时卡住。
    */
}

unsigned int MysqlUserRepo::create_user(const std::string& username,
                                        const std::string& password_hash) {
    MYSQL* conn = acquire_connection();
    if (!conn) return 0;

    // 这是一个只给当前函数用的临时辅助类型
    struct ScopedConn {
        MysqlUserRepo* self;        // 当前 MysqlUserRepo 对象
        MYSQL* conn;                // 当前拿到的 MySQL 连接
        ~ScopedConn() { self->release_connection(conn); }
    } scoped{this, conn};

    const std::string u = escape_sql(conn, username);       // 转义为符合SQL语法的字符串
    const std::string h = escape_sql(conn, password_hash);

    std::string sql =
        "INSERT INTO users (username, password_hash) VALUES ('" + u + "', '" + h + "')";

    if (mysql_query(conn, sql.c_str()) != 0) {
        unsigned int err = mysql_errno(conn);
        if (err == ER_DUP_ENTRY) return 0;
        return 0;
    }

    return static_cast<unsigned int>(mysql_insert_id(conn));
}

std::optional<UserRecord> MysqlUserRepo::get_by_username(const std::string& username) {
    auto start = std::chrono::high_resolution_clock::now();
    MYSQL* conn = acquire_connection();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "acquire_connection() 执行时间: " << duration.count() << " 微秒" << std::endl;
    if (!conn) return std::nullopt;

    struct ScopedConn {
        MysqlUserRepo* self;
        MYSQL* conn;
        ~ScopedConn() { self->release_connection(conn); }
    } scoped{this, conn};

    const std::string u = escape_sql(conn, username);
    std::string sql =
        "SELECT id, username, password_hash "
        "FROM users WHERE username='" + u + "' LIMIT 1";

    start = std::chrono::high_resolution_clock::now();
    if (mysql_query(conn, sql.c_str()) != 0) {  // 让 conn 这个 MySQL 连接去执行刚才那条 sql
        return std::nullopt;
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "mysql_query() 执行时间: " << duration.count() << " 微秒" << std::endl;

    MYSQL_RES* res = mysql_store_result(conn);  // 把刚才查询出来的结果，从 MySQL 连接里取出来，放到一个结果集对象 res 里
    if (!res) return std::nullopt;

    MYSQL_ROW row = mysql_fetch_row(res);       // 从结果集 res 里取出当前的一行记录
    if (!row) {
        mysql_free_result(res);
        return std::nullopt;
    }

    UserRecord r;
    r.id = static_cast<unsigned int>(std::stoul(row[0]));
    r.username = row[1] ? row[1] : "";
    r.password_hash = row[2] ? row[2] : "";
    mysql_free_result(res);
    return r;
}

std::optional<UserRecord> MysqlUserRepo::get_by_id(unsigned int user_id) {
    MYSQL* conn = acquire_connection();
    if (!conn) return std::nullopt;

    struct ScopedConn {
        MysqlUserRepo* self;
        MYSQL* conn;
        ~ScopedConn() { self->release_connection(conn); }
    } scoped{this, conn};

    std::string sql =
        "SELECT id, username, password_hash "
        "FROM users WHERE id=" + std::to_string(user_id) + " LIMIT 1";

    if (mysql_query(conn, sql.c_str()) != 0) {
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return std::nullopt;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        return std::nullopt;
    }

    UserRecord r;
    r.id = static_cast<unsigned int>(std::stoul(row[0]));
    r.username = row[1] ? row[1] : "";
    r.password_hash = row[2] ? row[2] : "";
    mysql_free_result(res);
    return r;
}

bool MysqlUserRepo::update_password_hash(unsigned int user_id, const std::string& new_password_hash) {
    MYSQL* conn = acquire_connection();
    if (!conn) return false;

    struct ScopedConn {
        MysqlUserRepo* self;
        MYSQL* conn;
        ~ScopedConn() { self->release_connection(conn); }
    } scoped{this, conn};

    const std::string hash = escape_sql(conn, new_password_hash);
    std::string sql =
        "UPDATE users SET password_hash='" + hash + "' WHERE id=" + std::to_string(user_id) + " LIMIT 1";

    if (mysql_query(conn, sql.c_str()) != 0) {
        return false;
    }
    return mysql_affected_rows(conn) == 1;
}

std::vector<std::string> MysqlUserRepo::list_all_usernames() {
    MYSQL* conn = acquire_connection();
    if (!conn) return {};

    struct ScopedConn {
        MysqlUserRepo* self;
        MYSQL* conn;
        ~ScopedConn() { self->release_connection(conn); }
    } scoped{this, conn};

    const char* sql = "SELECT username FROM users";
    if (mysql_query(conn, sql) != 0) {
        return {};
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return {};

    std::vector<std::string> usernames;
    usernames.reserve(static_cast<std::size_t>(mysql_num_rows(res)));

    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        usernames.emplace_back(row[0] ? row[0] : "");
    }

    mysql_free_result(res);
    return usernames;
}
