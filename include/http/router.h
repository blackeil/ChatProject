#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "http/http_request.h"
#include "http/http_response.h"

namespace http {

class Router {
public:
    using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;

    void Get(const std::string& path, Handler handler);
    void Post(const std::string& path, Handler handler);

    bool route(const HttpRequest& req, HttpResponse& res) const;

private:
    std::unordered_map<std::string, Handler> handlers_;

    static std::string make_key(const std::string& method, const std::string& path);
};

}  // namespace http
