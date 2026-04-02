#pragma once

#include <string>

namespace http {

struct HttpResponse {
    int status = 200;
    std::string content_type = "text/plain";
    std::string body;

    void set_content(const std::string& content, const std::string& type);
};

}  // namespace http
