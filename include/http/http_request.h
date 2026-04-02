#pragma once

#include <string>
#include <unordered_map>

namespace http {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;

    std::string header(const std::string& name) const;
};

}  // namespace http
