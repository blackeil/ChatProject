#pragma once

#include <string>
#include "json.hpp"

struct ServiceResult {
    int code = 0;
    std::string msg = "ok";
    nlohmann::json data = nlohmann::json::object();
};