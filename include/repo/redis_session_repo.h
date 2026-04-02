#pragma once

#include <string>
#include <optional>
#include <iostream>
#include <mutex>

struct redisContext;

struct SessionInfo {
    unsigned int user_id;
    std::string username;
};

class RedisSessionRepo {
public:
    RedisSessionRepo(const std::string& host, int port);
    ~RedisSessionRepo();

    bool connect();
    bool is_connected() const;

    // 生成随机 token
    std::string generate_token();

    // 存 session：key=session:TOKEN，value=JSON{user_id,username}，TTL 秒后过期
    bool set_session(const std::string& token, unsigned int user_id,
                     const std::string& username, int ttl_seconds);

    // 查 session，不存在或已过期返回 nullopt
    std::optional<SessionInfo> get_session(const std::string& token);

    // 删除 session，登出时使用
    bool delete_session(const std::string& token);

    // 刷新 session 过期时间（滑动过期）
    bool refresh_session_ttl(const std::string& token, int ttl_seconds);

private:
    // 不再持有共享连接：每次 Redis 操作都创建并释放独立连接，规避并发共享连接问题。
    bool reachable_ = false;
    std::string host_;
    int port_;

    redisContext* open_connection() const;
};
/*
typedef struct redisContext {
    int err;                    // 错误标志，0 表示无错误
    char errstr[128];           // 错误描述字符串
    int fd;                     // 底层 socket 文件描述符
    int flags;                  // 连接标志（如 REDIS_BLOCK 表示阻塞模式）
    char *obuf;                 // 输出缓冲区，待发送的命令暂存于此
    redisReader *reader;        // 协议解析器，用于解析 Redis 响应
    enum redisConnectionType connection_type; // 连接类型（TCP/Unix Socket）
    struct timeval *timeout;    // 超时设置
    struct {
        char *host;
        int port;
    } tcp;                      // TCP 连接配置
    struct {
        char *path;
    } unix_sock;                // Unix 域套接字配置
} redisContext;

建立连接：
redisContext *redisConnect(const char *ip, int port);
redisContext *redisConnectWithTimeout(const char *ip, int port, const struct timeval tv);
发送命令：
void *redisCommand(redisContext *c, const char *format, ...);
释放连接：
void redisFree(redisContext *c);

‌redisReply‌ 是 Redis C 客户端库 ‌hiredis‌ 中用于封装 Redis 命令执行返回结果的核心结构体。
它定义了 Redis 服务器响应的统一格式，开发者需根据其 type 字段判断返回类型，并据此访问对应的数据成员。
typedef struct redisReply {
    int type;                    // 返回值类型（如 REDIS_REPLY_STRING、REDIS_REPLY_INTEGER 等）
    long long integer;           // 当 type 为 REDIS_REPLY_INTEGER 时使用的整数值
    size_t len;                  // 字符串长度（适用于 REDIS_REPLY_STRING、REDIS_REPLY_ERROR 等）
    char *str;                   // 指向字符串内容的指针（用于状态、错误、字符串等类型）
    size_t elements;             // 数组元素个数（仅当 type 为 REDIS_REPLY_ARRAY 时有效）
    struct redisReply **element; // 指向子回复数组的指针（用于嵌套或批量返回）
} redisReply;
‌必须手动释放‌：freeReplyObject(reply)
*/

/*
SessionInfo：存 user_id、username，和 MySQL 的 UserRecord 类似但只包含 session 需要的信息
generate_token：用随机数生成 token
set_session：用 Redis SETEX，key 带 TTL
get_session：用 GET 查，查不到就返回 nullopt
redisContext*：hiredis 内部类型，用前向声明，不在头文件里 include hiredis
*/
