#include <iostream>
#include <chrono>
#include "config/config.h"
#include "httplib.h"
#include "auth/handlers_auth.h"
#include "user/handlers_user.h"
#include "repo/mysql_user_repo.h"
#include "repo/redis_session_repo.h"
#include "username_bloom_filter.h"

int main() {
    Config cfg;
    if(!cfg.load("config/config.json")) {
        std::cerr << "failed to load config\n";
        return 1;
    }

    MysqlUserRepo user_repo(
        cfg.mysql_host(),
        cfg.mysql_port(),
        cfg.mysql_database(),
        cfg.mysql_user(),
        cfg.mysql_password(),
        cfg.mysql_pool_size()
    );
    if (!user_repo.connect()) {
        std::cerr << "failed to connect mysql\n";
        return 1;
    }

    // 新增：全量拉取用户名，构建 Bloom Filter
    std::vector<std::string> usernames = user_repo.list_all_usernames();
    UsernameBloomFilter username_bf(usernames.size() + 1000, 0.001);
    username_bf.rebuild(usernames);

    RedisSessionRepo redis_repo(cfg.redis_host(), cfg.redis_port());
    if(!redis_repo.connect()) {
        std::cerr << "Failed to connect redis\n";
        return 1;
    }

    httplib::Server svr;    //创建一个 HTTP 服务器对象

    svr.Post("/api/register", [&user_repo, cfg, &username_bf](const httplib::Request& req, httplib::Response& res) {
        handle_register(req, res, &user_repo, &username_bf, cfg.bcrypt_cost());
    }); // 当受到 POST /api/register、就执行后面这个 lambda，lambda再去调用handle_register
    svr.Post("/api/login", [&user_repo, &redis_repo, cfg, &username_bf](const httplib::Request& req, httplib::Response& res) {
        handle_login(req, res, &user_repo, &redis_repo, &username_bf, cfg.token_ttl_seconds());
    });
    svr.Get("/api/profile", [&redis_repo, cfg](const httplib::Request& req, httplib::Response& res) {
        handle_profile(req, res, &redis_repo, cfg.token_ttl_seconds());
    });
    svr.Get("/api/user/me", [&user_repo, &redis_repo, cfg](const httplib::Request& req, httplib::Response& res) {
        handle_user_me(req, res, &user_repo, &redis_repo, cfg.token_ttl_seconds());
    });
    svr.Post("/api/user/password", [&user_repo, &redis_repo, cfg, &username_bf](const httplib::Request& req, httplib::Response& res) {
        handle_change_password(req, res, &user_repo, &redis_repo, cfg.bcrypt_cost());
    });
    svr.Post("/api/logout", [&redis_repo](const httplib::Request& req, httplib::Response& res) {
        handle_logout(req, res, &redis_repo);
    });

    std::cout << "listening on " << cfg.server_host()
              << ":" << cfg.server_port() << std::endl;
    svr.listen(cfg.server_host().c_str(), cfg.server_port());

    return 0;
}

/*
配置类能读 server/mysql/auth 配置（为了将来用）。
HTTP + JSON 的两个接口还是假注册/假登录。
MysqlUserRepo 的代码已经在项目里，但还没被调用。
*/
