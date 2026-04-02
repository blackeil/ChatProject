#include "http/http_api_response.h"

#include "api_response.h"

namespace http {

void respond(HttpResponse& res, int http_status, int code, const std::string& msg,
             const nlohmann::json& data) {
    nlohmann::json body;
    body["code"] = code;
    body["msg"] = msg;
    body["data"] = data.is_null() ? nlohmann::json::object() : data;
    res.status = http_status;
    res.set_content(body.dump(2) + "\n", "application/json");
}

void respond_code(HttpResponse& res, int code) {
    const int mapped = api::default_http_status(code);
    respond(res, mapped, mapped, api::default_message(code), nlohmann::json::object());
}

void respond_code_data(HttpResponse& res, int code, const nlohmann::json& data) {
    const int mapped = api::default_http_status(code);
    respond(res, mapped, mapped, api::default_message(code), data);
}

void respond_code_msg(HttpResponse& res, int code, const std::string& msg,
                      const nlohmann::json& data) {
    const int mapped = api::default_http_status(code);
    respond(res, mapped, mapped, msg, data);
}

}  // namespace http
