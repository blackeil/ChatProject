#include "http/router.h"

namespace http {

std::string HttpRequest::header(const std::string& name) const {
    auto it = headers.find(name);
    if (it == headers.end()) return {};
    return it->second;
}

void HttpResponse::set_content(const std::string& content, const std::string& type) {
    body = content;
    content_type = type;
}

void Router::Get(const std::string& path, Handler handler) {
    handlers_[make_key("GET", path)] = std::move(handler);
}

void Router::Post(const std::string& path, Handler handler) {
    handlers_[make_key("POST", path)] = std::move(handler);
}

bool Router::route(const HttpRequest& req, HttpResponse& res) const {
    auto it = handlers_.find(make_key(req.method, req.path));
    if (it == handlers_.end()) {
        return false;
    }
    it->second(req, res);
    return true;
}

std::string Router::make_key(const std::string& method, const std::string& path) {
    return method + " " + path;
}

}  // namespace http
