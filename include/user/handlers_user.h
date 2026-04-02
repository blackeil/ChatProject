#pragma once

#include "http/http_request.h"
#include "http/http_response.h"

struct MysqlUserRepo;
struct RedisSessionRepo;

void handle_user_me(const http::HttpRequest& req, http::HttpResponse& res,
                    MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                    int token_ttl_seconds);

void handle_change_password(const http::HttpRequest& req, http::HttpResponse& res,
                            MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                            int bcrypt_cost);
