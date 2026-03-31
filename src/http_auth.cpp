#include "../include/http_auth.h"

std::string extract_token_from_request(const httplib::Request& req) {
    const auto auth = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.size() > prefix.size() && auth.rfind(prefix, 0) == 0) {
        return auth.substr(prefix.size());
    }

    // const auto x_token = req.get_header_value("X-Session-Token");
    // if (!x_token.empty()) return x_token;

    // if (req.has_param("token")) return req.get_param_value("token");
    return {};
}

