#include "../include/api_response.h"

namespace api {

int default_http_status(int code) {
    if (code == CODE_OK) return 200;
    //  case标签必须要求常量表达式
    switch (code) {
        case CODE_MISSING_PARAM:
        case CODE_USERNAME_INVALID:
        case CODE_INVALID_CREDENTIAL:
        case CODE_INVALID_JSON:
            return 400; // 请求错误，请求报文存在语法错误
        case CODE_UNAUTHORIZED_MISSING_TOKEN:
        case CODE_UNAUTHORIZED_INVALID_SESSION:
            return 401; // 未认证，token无效
        case CODE_USERNAME_EXISTS:
            return 409; // 重复注册，请求和当前资源状态冲突
        case CODE_SERVICE_UNAVAILABLE:
        case CODE_HASH_FAILED:
        case CODE_LOGOUT_FAILED:
            return 500; // 服务器内部错误
        default:
            return 500;
    }
}

std::string default_message(int code) {
    switch (code) {
        case CODE_OK:
            return "ok";
        case CODE_MISSING_PARAM:
            return "missing required parameters";
        case CODE_USERNAME_INVALID:
            return "invalid username or password format";
        case CODE_INVALID_CREDENTIAL:
            return "invalid username or password";
        case CODE_INVALID_JSON:
            return "invalid json";
        case CODE_USERNAME_EXISTS:
            return "username already exists";
        case CODE_UNAUTHORIZED_MISSING_TOKEN:
            return "missing token";
        case CODE_UNAUTHORIZED_INVALID_SESSION:
            return "session expired or invalid";
        case CODE_SERVICE_UNAVAILABLE:
            return "service unavailable";
        case CODE_HASH_FAILED:
            return "password hash failed";
        case CODE_LOGOUT_FAILED:
            return "logout failed";
        default:
            return "internal error";
    }
}

void respond(httplib::Response& res, int http_status, int code,
             const std::string& msg, const nlohmann::json& data) {
    nlohmann::json body;
    body["code"] = code;
    body["msg"] = msg;
    body["data"] = data.is_null() ? nlohmann::json::object() : data;
    res.status = http_status;
    res.set_content(body.dump(2) + "\n", "application/json");
}

void respond_code(httplib::Response& res, int code) {
    std::cout << "respond_code" << std::endl;
    respond(res, default_http_status(code), default_http_status(code), default_message(code), nlohmann::json::object());
}

void respond_code_data(httplib::Response& res, int code, const nlohmann::json& data) {
    std::cout << "respond_code_data" << std::endl;
    respond(res, default_http_status(code), default_http_status(code), default_message(code), data);
}

void respond_code_msg(httplib::Response& res, int code, const std::string& msg,
                      const nlohmann::json& data) {
    std::cout << "respond_code_msg" << std::endl;
    respond(res, default_http_status(code), default_http_status(code), msg, data);
}

}  // namespace api
