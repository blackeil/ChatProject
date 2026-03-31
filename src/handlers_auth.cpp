#include "../include/handlers_auth.h"

#include "../include/api_response.h"
#include "../include/auth_service.h"
#include "../include/http_auth.h"
#include "../include/json.hpp"

#include <exception>
#include <string>

using json = nlohmann::json;

static void respond_service_result(httplib::Response& res, const ServiceResult& result) {
    api::respond_code_msg(res, result.code, result.msg, result.data);
}

void handle_register(const httplib::Request& req, httplib::Response& res,
                     MysqlUserRepo* repo, int bcrypt_cost) {
    try {
        json j = json::parse(req.body);
        if (!j.contains("username") || !j.contains("password")) {
            api::respond_code_msg(res, api::CODE_MISSING_PARAM, "missing username or password");
            return;
        }

        const std::string username = j["username"].get<std::string>();
        const std::string password = j["password"].get<std::string>();
        respond_service_result(res, auth_service::register_user(repo, username, password, bcrypt_cost));
        std::cout << "sucesss register new user, username :" << username << " password: " << password << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        api::respond_code(res, api::CODE_INVALID_JSON);
    }
}

void handle_login(const httplib::Request& req, httplib::Response& res, MysqlUserRepo* repo,
                  RedisSessionRepo* redis_repo, int token_ttl_seconds) {
    try {
        json j = json::parse(req.body);
        if (!j.contains("username") || !j.contains("password")) {
            api::respond_code_msg(res, api::CODE_MISSING_PARAM, "missing username or password");
            return;
        }

        const std::string username = j["username"].get<std::string>();
        const std::string password = j["password"].get<std::string>();
        respond_service_result(
            res, auth_service::login_user(repo, redis_repo, username, password, token_ttl_seconds));
    } catch (const std::exception&) {
        api::respond_code(res, api::CODE_INVALID_JSON);
    }
}

void handle_profile(const httplib::Request& req, httplib::Response& res,
                    RedisSessionRepo* redis_repo, int token_ttl_seconds) {
    auto session = require_auth_session(req, res, redis_repo, token_ttl_seconds);
    if (!session.has_value()) return;

    json data;
    data["user"] = {
        {"id", session->user_id},
        {"username", session->username}
    };
    api::respond_code_data(res, api::CODE_OK, data);
}

void handle_logout(const httplib::Request& req, httplib::Response& res,
                   RedisSessionRepo* redis_repo) {
    auto session = require_auth_session(req, res, redis_repo, 0);
    if (!session.has_value()) return;

    const std::string token = extract_token_from_request(req);
    ServiceResult result = auth_service::logout_user(redis_repo, token);
    if (result.code != api::CODE_OK) {
        respond_service_result(res, result);
        return;
    }

    result.data["user"] = {
        {"id", session->user_id},
        {"username", session->username}
    };
    respond_service_result(res, result);
}

std::optional<SessionInfo> require_auth_session(const httplib::Request& req, httplib::Response& res,
                                                RedisSessionRepo* redis_repo, int token_ttl_seconds) {
    const std::string token = extract_token_from_request(req);
    ServiceResult error;
    auto session = auth_service::authenticate_session(redis_repo, token, token_ttl_seconds, error);
    if (!session.has_value()) {
        api::respond_code_msg(res, error.code, error.msg);
        return std::nullopt;
    }
    return session;
}

