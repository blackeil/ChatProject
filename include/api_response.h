#pragma once

#include <string>
#include "../cpp-httplib/httplib.h"
#include "json.hpp"

namespace api {

constexpr int CODE_OK = 0;

constexpr int CODE_MISSING_PARAM = 1000;
constexpr int CODE_USERNAME_INVALID = 1001;
constexpr int CODE_INVALID_CREDENTIAL = 1002;
constexpr int CODE_INVALID_JSON = 1003;
constexpr int CODE_USERNAME_EXISTS = 1004;

constexpr int CODE_UNAUTHORIZED_MISSING_TOKEN = 1007;
constexpr int CODE_UNAUTHORIZED_INVALID_SESSION = 1008;

constexpr int CODE_SERVICE_UNAVAILABLE = 5000;
constexpr int CODE_HASH_FAILED = 5001;
constexpr int CODE_LOGOUT_FAILED = 5002;

int default_http_status(int code);
std::string default_message(int code);

void respond(httplib::Response& res, int http_status, int code,
             const std::string& msg,
             const nlohmann::json& data = nlohmann::json::object());

void respond_code(httplib::Response& res, int code);

void respond_code_data(httplib::Response& res, int code,
                       const nlohmann::json& data);

void respond_code_msg(httplib::Response& res, int code, const std::string& msg,
                      const nlohmann::json& data = nlohmann::json::object());

}  // namespace api
