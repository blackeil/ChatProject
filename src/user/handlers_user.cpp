#include "user/handlers_user.h"

#include "api_response.h"
#include "auth/handlers_auth.h"
#include "http/http_api_response.h"
#include "json.hpp"
#include "user/user_service.h"

#include <exception>
#include <string>

using json = nlohmann::json;

static std::string extract_token_from_request(const http::HttpRequest& req) {
    const std::string auth = req.header("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.size() > prefix.size() && auth.rfind(prefix, 0) == 0) {
        return auth.substr(prefix.size());
    }
    return {};
}

static void respond_service_result(http::HttpResponse& res, const ServiceResult& result) {
    http::respond_code_msg(res, result.code, result.msg, result.data);
}

void handle_user_me(const http::HttpRequest& req, http::HttpResponse& res,
                    MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                    int token_ttl_seconds) {
    auto session = require_auth_session(req, res, redis_repo, token_ttl_seconds);
    if (!session.has_value()) return;

    respond_service_result(res, user_service::get_user_me(user_repo, *session));
}

void handle_change_password(const http::HttpRequest& req, http::HttpResponse& res,
                            MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                            int bcrypt_cost) {
    auto session = require_auth_session(req, res, redis_repo, 0);
    if (!session.has_value()) return;

    try {
        json j = json::parse(req.body);
        if (!j.contains("old_password") || !j.contains("new_password")) {
            http::respond_code_msg(res, api::CODE_MISSING_PARAM, "missing old_password or new_password");
            return;
        }

        const std::string old_password = j["old_password"].get<std::string>();
        const std::string new_password = j["new_password"].get<std::string>();
        const std::string token = extract_token_from_request(req);

        respond_service_result(res, user_service::change_password(
                                        user_repo, redis_repo, *session, token,
                                        old_password, new_password, bcrypt_cost));
    } catch (const std::exception&) {
        http::respond_code(res, api::CODE_INVALID_JSON);
    }
}
