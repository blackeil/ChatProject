#pragma once

#include <optional>
#include "../cpp-httplib/httplib.h"
#include "redis_session_repo.h"

struct MysqlUserRepo;

void handle_register(const httplib::Request& req, httplib::Response& res,
                     MysqlUserRepo* repo, int bcrypt_cost);
void handle_login(const httplib::Request& req, httplib::Response& res, 
                  MysqlUserRepo* repo, RedisSessionRepo* redis_repo,
                  int token_ttl_seconds);

// 受保护接口示例：验证 session 后返回当前用户
void handle_profile(const httplib::Request& req, httplib::Response& res,
                    RedisSessionRepo* redis_repo, int token_ttl_seconds);

void handle_logout(const httplib::Request& req, httplib::Response& res, 
                    RedisSessionRepo* redis_repo);
// 统一 session 校验逻辑，后续接口复用它，不需要再查 MySQL
std::optional<SessionInfo> require_auth_session(const httplib::Request& req, httplib::Response& res,
                                                RedisSessionRepo* redis_repo, int token_ttl_seconds);
