#pragma once

#include <optional>
#include <string>
#include "repo/redis_session_repo.h"
#include "service_result.h"

class MysqlUserRepo;
class RedisSessionRepo;

namespace auth_service {

ServiceResult register_user(MysqlUserRepo* user_repo, const std::string& username,
                            const std::string& password, int bcrypt_cost);

ServiceResult login_user(MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                         const std::string& username, const std::string& password,
                         int token_ttl_seconds);

ServiceResult logout_user(RedisSessionRepo* redis_repo, const std::string& token);

// 鉴权成功返回 SessionInfo；失败时把错误写入 error
std::optional<SessionInfo> authenticate_session(RedisSessionRepo* redis_repo, const std::string& token,
                                                int token_ttl_seconds, ServiceResult& error);

}  // namespace auth_service

