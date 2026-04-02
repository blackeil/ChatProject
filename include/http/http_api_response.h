#pragma once

#include <string>

#include "http/http_response.h"
#include "json.hpp"

namespace http {

void respond(HttpResponse& res, int http_status, int code, const std::string& msg,
             const nlohmann::json& data = nlohmann::json::object());

void respond_code(HttpResponse& res, int code);

void respond_code_data(HttpResponse& res, int code, const nlohmann::json& data);

void respond_code_msg(HttpResponse& res, int code, const std::string& msg,
                      const nlohmann::json& data = nlohmann::json::object());

}  // namespace http
