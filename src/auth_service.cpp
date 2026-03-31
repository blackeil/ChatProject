#include "../include/auth_service.h"

#include "../include/api_response.h"
#include "../include/mysql_user_repo.h"
#include "../include/password_hash.h"
#include "../include/redis_session_repo.h"

namespace auth_service {

ServiceResult register_user(MysqlUserRepo* user_repo, const std::string& username,
                            const std::string& password, int bcrypt_cost) {
    if (!user_repo || !user_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "database unavailable"};
    }
    if (username.size() < 3 || username.size() > 32) {
        return {api::CODE_USERNAME_INVALID, "username length must be between 3 and 32"};
    }
    if (password.size() < 6 || password.size() > 64) {
        return {api::CODE_USERNAME_INVALID, "password length must be between 6 and 64"};
    }

    std::string hash = password_hash::hash(password, bcrypt_cost);
    if (hash.empty()) {
        return {api::CODE_HASH_FAILED, "password hash failed"};
    }

    unsigned int uid = user_repo->create_user(username, hash);
    if (uid == 0) {
        return {api::CODE_USERNAME_EXISTS, "username already exists"};
    }
    return {api::CODE_OK, "register success"};
}

ServiceResult login_user(MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                         const std::string& username, const std::string& password,
                         int token_ttl_seconds) {
    if (!user_repo || !user_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "database unavailable"};
    }
    if (!redis_repo || !redis_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "redis unavailable"};
    }
    if (username.empty() || password.empty()) {
        return {api::CODE_INVALID_CREDENTIAL, "invalid username or password"};
    }

    auto user = user_repo->get_by_username(username);
    if (!user.has_value()) {
        return {api::CODE_INVALID_CREDENTIAL, "invalid username or password"};
    }
    auto start = std::chrono::high_resolution_clock::now();
    if (!password_hash::verify(password, user->password_hash)) {
        return {api::CODE_INVALID_CREDENTIAL, "invalid username or password"};
    }
 
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "verify() 执行时间: " << duration.count() << " 微秒" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    std::string token = redis_repo->generate_token();

    if (!redis_repo->set_session(token, user->id, user->username, token_ttl_seconds)) {
        return {api::CODE_SERVICE_UNAVAILABLE, "save session failed"};
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "generate_token() + set_session() 执行时间: " << duration.count() << " 微秒" << std::endl;

    ServiceResult result{api::CODE_OK, "ok"};
    result.data["token"] = token;
    result.data["user"] = {
        {"id", user->id},
        {"username", user->username}
    };
    return result;
}

ServiceResult logout_user(RedisSessionRepo* redis_repo, const std::string& token) {
    if (!redis_repo || !redis_repo->is_connected()) {
        return {api::CODE_SERVICE_UNAVAILABLE, "redis unavailable"};
    }
    if (token.empty()) {
        return {api::CODE_UNAUTHORIZED_MISSING_TOKEN, "missing token"};
    }
    if (!redis_repo->delete_session(token)) {
        return {api::CODE_LOGOUT_FAILED, "logout failed"};
    }
    return {api::CODE_OK, "logout success"};
}

std::optional<SessionInfo> authenticate_session(RedisSessionRepo* redis_repo, const std::string& token,
                                                int token_ttl_seconds, ServiceResult& error) {
    if (!redis_repo || !redis_repo->is_connected()) {
        error = {api::CODE_SERVICE_UNAVAILABLE, "redis unavailable"};
        return std::nullopt;
    }
    if (token.empty()) {
        error = {api::CODE_UNAUTHORIZED_MISSING_TOKEN, "missing token"};
        return std::nullopt;
    }

    auto session = redis_repo->get_session(token);
    if (!session.has_value()) {
        error = {api::CODE_UNAUTHORIZED_INVALID_SESSION, "session expired or invalid"};
        return std::nullopt;
    }

    if (token_ttl_seconds > 0) {
        redis_repo->refresh_session_ttl(token, token_ttl_seconds);
    }
    return session;
}

}  // namespace auth_service

