#include "../include/user_service.h"

#include "../include/api_response.h"
#include "../include/mysql_user_repo.h"
#include "../include/password_hash.h"
#include "../include/redis_session_repo.h"

namespace user_service {

ServiceResult get_user_me(MysqlUserRepo* user_repo, const SessionInfo& session) {
    if (!user_repo || !user_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "database unavailable"};
    }

    auto user = user_repo->get_by_id(session.user_id);
    if (!user.has_value()) {
        return {api::CODE_UNAUTHORIZED_INVALID_SESSION, "invalid session user"};
    }

    ServiceResult result{api::CODE_OK, "ok"};
    result.data["user"] = {
        {"id", user->id},
        {"username", user->username}
    };
    return result;
}

ServiceResult change_password(MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                              const SessionInfo& session, const std::string& current_token,
                              const std::string& old_password, const std::string& new_password,
                              int bcrypt_cost) {
    if (!user_repo || !user_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "database unavailable"};
    }
    if (!redis_repo || !redis_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "redis unavailable"};
    }
    if (old_password.empty() || new_password.empty()) {
        return {api::CODE_INVALID_CREDENTIAL, "invalid credential"};
    }
    if (new_password.size() < 6 || new_password.size() > 64) {
        return {api::CODE_USERNAME_INVALID, "new_password length must be between 6 and 64"};
    }

    auto user = user_repo->get_by_id(session.user_id);
    if (!user.has_value()) {
        return {api::CODE_UNAUTHORIZED_INVALID_SESSION, "invalid session user"};
    }
    if (!password_hash::verify(old_password, user->password_hash)) {
        return {api::CODE_INVALID_CREDENTIAL, "invalid username or password"};
    }

    std::string new_hash = password_hash::hash(new_password, bcrypt_cost);
    if (new_hash.empty()) {
        return {api::CODE_HASH_FAILED, "password hash failed"};
    }
    if (!user_repo->update_password_hash(user->id, new_hash)) {
        return {api::CODE_SERVICE_UNAVAILABLE, "update password failed"};
    }

    if (!current_token.empty()) {
        redis_repo->delete_session(current_token);
    }
    return {api::CODE_OK, "password changed, please login again"};
}

}  // namespace user_service

