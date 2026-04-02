#include "auth/handlers_auth.h"

#include "auth/auth_service.h"
#include "api_response.h"
#include "http/http_api_response.h"
#include "json.hpp"

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

void handle_register(const http::HttpRequest& req, http::HttpResponse& res,
                     MysqlUserRepo* repo, UsernameBloomFilter* bf, int bcrypt_cost) {
    try {
        json j = json::parse(req.body);
        if (!j.contains("username") || !j.contains("password")) {
            http::respond_code_msg(res, api::CODE_MISSING_PARAM, "missing username or password");
            return;
        }

        const std::string username = j["username"].get<std::string>();
        const std::string password = j["password"].get<std::string>();
        ServiceResult res_tmp = auth_service::register_user(repo, username, password, bcrypt_cost);
        respond_service_result(res, res_tmp);
        if (res_tmp.code == api::CODE_OK && bf) {
            bf->add(username);
        }
        // std::cout << "sucesss register new user, username :" << username << "\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        http::respond_code(res, api::CODE_INVALID_JSON);
    }
}

void handle_login(const http::HttpRequest& req, http::HttpResponse& res, MysqlUserRepo* repo,
                  RedisSessionRepo* redis_repo, UsernameBloomFilter* bf, int token_ttl_seconds) {
    try {
        json j = json::parse(req.body);
        if (!j.contains("username") || !j.contains("password")) {
            http::respond_code_msg(res, api::CODE_MISSING_PARAM, "missing username or password");
            return;
        }

        const std::string username = j["username"].get<std::string>();
        const std::string password = j["password"].get<std::string>();
        // bloom filter 只做前置剪枝，最终存在性仍以 MySQL 查询为准。
        if (bf && !bf->possiblyContains(username)) {
            http::respond_code_msg(res, 1005, "布隆过滤器判断: invalid username or password");
            return;
        }
        respond_service_result(
            res, auth_service::login_user(repo, redis_repo, username, password, token_ttl_seconds));
    } catch (const std::exception&) {
        http::respond_code(res, api::CODE_INVALID_JSON);
    }
}

void handle_profile(const http::HttpRequest& req, http::HttpResponse& res,
                    RedisSessionRepo* redis_repo, int token_ttl_seconds) {
    auto session = require_auth_session(req, res, redis_repo, token_ttl_seconds);
    if (!session.has_value()) return;

    json data;
    data["user"] = {
        {"id", session->user_id},
        {"username", session->username}
    };
    http::respond_code_data(res, api::CODE_OK, data);
}

void handle_logout(const http::HttpRequest& req, http::HttpResponse& res,
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

std::optional<SessionInfo> require_auth_session(const http::HttpRequest& req, http::HttpResponse& res,
                                                RedisSessionRepo* redis_repo, int token_ttl_seconds) {
    const std::string token = extract_token_from_request(req);
    ServiceResult error;
    auto session = auth_service::authenticate_session(redis_repo, token, token_ttl_seconds, error);
    if (!session.has_value()) {
        http::respond_code_msg(res, error.code, error.msg);
        return std::nullopt;
    }
    return session;
}
