#pragma once

#include <string>
#include "redis_session_repo.h"
#include "service_result.h"

class MysqlUserRepo;
class RedisSessionRepo;

namespace user_service {

ServiceResult get_user_me(MysqlUserRepo* user_repo, const SessionInfo& session);

ServiceResult change_password(MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                              const SessionInfo& session, const std::string& current_token,
                              const std::string& old_password, const std::string& new_password,
                              int bcrypt_cost);

}  // namespace user_service

