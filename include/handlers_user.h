#pragma once

#include "../cpp-httplib/httplib.h"

struct MysqlUserRepo;
struct RedisSessionRepo;

void handle_user_me(const httplib::Request& req, httplib::Response& res,
                    MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                    int token_ttl_seconds);

void handle_change_password(const httplib::Request& req, httplib::Response& res,
                            MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                            int bcrypt_cost);
