#include "repo/redis_session_repo.h"

#include "json.hpp"
#include <hiredis/hiredis.h>

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/time.h>

RedisSessionRepo::RedisSessionRepo(const std::string& host, int port)
    : host_(host), port_(port) {}

RedisSessionRepo::~RedisSessionRepo() = default;

redisContext* RedisSessionRepo::open_connection() const {
    struct timeval tv{5, 0};
    return redisConnectWithTimeout(host_.c_str(), port_, tv);
}

bool RedisSessionRepo::is_connected() const {
    return reachable_;
}

bool RedisSessionRepo::connect() {
    // connect() 现在只做可达性检查，不保存共享连接。
    redisContext* conn = open_connection();
    if (!conn) {
        reachable_ = false;
        std::cerr << "Redis: connect returned null\n";
        return false;
    }
    if (conn->err) {
        reachable_ = false;
        std::cerr << "Redis connect error: " << conn->errstr << "\n";
        redisFree(conn);
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(conn, "PING"));
    bool ok = reply && reply->type == REDIS_REPLY_STATUS && reply->str &&
              strcmp(reply->str, "PONG") == 0;
    if (reply) freeReplyObject(reply);
    redisFree(conn);

    reachable_ = ok;
    if (!ok) std::cerr << "Redis: PING did not return PONG\n";
    return ok;
}

std::string RedisSessionRepo::generate_token() {
    unsigned char buf[32];
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom.read(reinterpret_cast<char*>(buf), 32)) return {};

    std::ostringstream os;
    for (int i = 0; i < 32; ++i) {
        os << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(buf[i]);
    }
    return os.str();
}

bool RedisSessionRepo::set_session(const std::string& token, unsigned int user_id,
                                   const std::string& username, int ttl_seconds) {
    if (token.empty() || ttl_seconds <= 0) return false;

    redisContext* conn = open_connection();
    if (!conn || conn->err) {
        if (conn) redisFree(conn);
        reachable_ = false;
        return false;
    }
    reachable_ = true;

    nlohmann::json j;
    j["user_id"] = user_id;
    j["username"] = username;

    std::string val = j.dump();
    std::string key = "session:" + token;
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(conn, "SETEX %s %d %b", key.c_str(), ttl_seconds, val.c_str(), val.size()));

    if (!reply) {
        redisFree(conn);
        reachable_ = false;
        return false;
    }

    bool ok = reply->type == REDIS_REPLY_STATUS && reply->str &&
              strcasecmp(reply->str, "OK") == 0;
    freeReplyObject(reply);
    redisFree(conn);
    return ok;
}

std::optional<SessionInfo> RedisSessionRepo::get_session(const std::string& token) {
    if (token.empty()) return std::nullopt;

    redisContext* conn = open_connection();
    if (!conn || conn->err) {
        if (conn) redisFree(conn);
        reachable_ = false;
        return std::nullopt;
    }
    reachable_ = true;

    std::string key = "session:" + token;
    redisReply* reply = static_cast<redisReply*>(redisCommand(conn, "GET %s", key.c_str()));
    if (!reply) {
        redisFree(conn);
        reachable_ = false;
        return std::nullopt;
    }

    if (reply->type != REDIS_REPLY_STRING || !reply->str) {
        freeReplyObject(reply);
        redisFree(conn);
        return std::nullopt;
    }

    std::string val(reply->str, reply->len);
    freeReplyObject(reply);
    redisFree(conn);

    try {
        auto j = nlohmann::json::parse(val);
        SessionInfo info;
        info.user_id = j.at("user_id").get<unsigned int>();
        info.username = j.at("username").get<std::string>();
        return info;
    } catch (...) {
        return std::nullopt;
    }
}

bool RedisSessionRepo::delete_session(const std::string& token) {
    if (token.empty()) return false;

    redisContext* conn = open_connection();
    if (!conn || conn->err) {
        if (conn) redisFree(conn);
        reachable_ = false;
        return false;
    }
    reachable_ = true;

    std::string key = "session:" + token;
    redisReply* reply = static_cast<redisReply*>(redisCommand(conn, "DEL %s", key.c_str()));
    if (!reply) {
        redisFree(conn);
        reachable_ = false;
        return false;
    }

    // 保持原语义：只要 DEL 返回整型结果即视为命令执行成功。
    bool ok = reply->type == REDIS_REPLY_INTEGER;
    freeReplyObject(reply);
    redisFree(conn);
    return ok;
}

bool RedisSessionRepo::refresh_session_ttl(const std::string& token, int ttl_seconds) {
    if (token.empty() || ttl_seconds <= 0) return false;

    redisContext* conn = open_connection();
    if (!conn || conn->err) {
        if (conn) redisFree(conn);
        reachable_ = false;
        return false;
    }
    reachable_ = true;

    std::string key = "session:" + token;
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(conn, "EXPIRE %s %d", key.c_str(), ttl_seconds));
    if (!reply) {
        redisFree(conn);
        reachable_ = false;
        return false;
    }

    bool ok = reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    freeReplyObject(reply);
    redisFree(conn);
    return ok;
}

