#pragma once

#include <optional>
#include "http/http_request.h"
#include "http/http_response.h"
#include "repo/redis_session_repo.h"
#include "username_bloom_filter.h"

struct MysqlUserRepo;
struct UsernameBloomFilter;

void handle_register(const http::HttpRequest& req, http::HttpResponse& res,
                     MysqlUserRepo* repo, UsernameBloomFilter* bf, 
                     int bcrypt_cost);
void handle_login(const http::HttpRequest& req, http::HttpResponse& res, 
                  MysqlUserRepo* repo, RedisSessionRepo* redis_repo,
                  UsernameBloomFilter* bf, int token_ttl_seconds);

// 受保护接口示例：验证 session 后返回当前用户
void handle_profile(const http::HttpRequest& req, http::HttpResponse& res,
                    RedisSessionRepo* redis_repo, int token_ttl_seconds);

void handle_logout(const http::HttpRequest& req, http::HttpResponse& res, 
                    RedisSessionRepo* redis_repo);
// 统一 session 校验逻辑，后续接口复用它，不需要再查 MySQL
std::optional<SessionInfo> require_auth_session(const http::HttpRequest& req, http::HttpResponse& res,
                                                RedisSessionRepo* redis_repo, int token_ttl_seconds);
