#include "../include/handlers_user.h"

#include "../include/api_response.h"
#include "../include/handlers_auth.h"
#include "../include/http_auth.h"
#include "../include/json.hpp"
#include "../include/user_service.h"

#include <exception>
#include <string>

using json = nlohmann::json;

static void respond_service_result(httplib::Response& res, const ServiceResult& result) {
    api::respond_code_msg(res, result.code, result.msg, result.data);
}

void handle_user_me(const httplib::Request& req, httplib::Response& res,
                    MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                    int token_ttl_seconds) {
    auto session = require_auth_session(req, res, redis_repo, token_ttl_seconds);
    if (!session.has_value()) return;

    respond_service_result(res, user_service::get_user_me(user_repo, *session));
}

void handle_change_password(const httplib::Request& req, httplib::Response& res,
                            MysqlUserRepo* user_repo, RedisSessionRepo* redis_repo,
                            int bcrypt_cost) {
    auto session = require_auth_session(req, res, redis_repo, 0);
    if (!session.has_value()) return;

    try {
        json j = json::parse(req.body);
        if (!j.contains("old_password") || !j.contains("new_password")) {
            api::respond_code_msg(res, api::CODE_MISSING_PARAM, "missing old_password or new_password");
            return;
        }

        const std::string old_password = j["old_password"].get<std::string>();
        const std::string new_password = j["new_password"].get<std::string>();
        const std::string token = extract_token_from_request(req);

        respond_service_result(res, user_service::change_password(
                                        user_repo, redis_repo, *session, token,
                                        old_password, new_password, bcrypt_cost));
    } catch (const std::exception&) {
        api::respond_code(res, api::CODE_INVALID_JSON);
    }
}

