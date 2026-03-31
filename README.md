一个基于 HTTP 的用户认证与会话管理后端服务
HTTP库 使用cpp-httplib
wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
或 git clone https://github.com/yhirose/cpp-httplib.git

创建数据库：
CREATE DATABASE IF NOT EXISTS safe_login
  DEFAULT CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE safe_login;

CREATE TABLE users (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(64) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

编译：
cd /home/crystal/chat-project-based-on-ubuntu-master/safe-login-server
mkdir build
cd build
cmake ..
cmake --build .
./safe_login_server

cmake --build build 等同于 cd build && make
---
cmake -S . -B build
cmake --build build
---

在 Ubuntu 引入 bcrypt
git clone https://github.com/trusch/libbcrypt.git
cd libbcrypt

mkdir build
cd build
cmake ..
make
sudo make install

安装后
头文件：/usr/local/include/bcrypt
库文件：/usr/local/lib/libbcrypt.a

或者使用系统 C 库自带 libcrypt



整体流程

启动：读 config.json → 用里面的 mysql 配置建 MysqlUserRepo → connect() 连上 MySQL → 注册两个路由，用 lambda 把 user_repo 传进 handler。
注册：解析 JSON → 校验长度 → 用 bcrypt 哈希密码 → create_user(username, hash) 写库；若用户名已存在（唯一键冲突）返回 1004。
登录：解析 JSON → get_by_username(username) 查库 → 用 bcrypt 的 verify(明文, 存库的 hash) 校验 → 通过则返回 dummy token + 用户信息。
为什么用 lambda 传 repo

httplib 的 Post(path, handler) 只接受 (Request, Response) 两个参数，不能直接传 user_repo。
用 [&user_repo] 捕获，在 lambda 里再调 handle_register(req, res, &user_repo)，这样 handler 能拿到 DB，又不用改库的 API。面试可以说：“用 lambda 做依赖注入，把 repo 注入到 handler，方便单测时换成 mock。”
密码为什么不能明文存

库被拖库或日志泄露时，明文密码会直接暴露；bcrypt 是单向哈希，只能校验不能反推。面试可以说：“用 bcrypt，cost 设 12，兼顾安全和 CPU；校验用 constant-time 比较，避免时序攻击。”（我们当前用的是 strcmp，若面试追问可以说“生产会用 constant-time 比较”。）
Q1:为什么不担心“比较过程被偷”
比较发生在服务器内存里，攻击者能读服务器内存，已经是比密码泄露更严重的问题
真正的薄弱环节是：客户端 → 服务器 的传输。
如果走 HTTP，密码明文在网络里被 sniff 的风险很大
生产环境要用 HTTPS，加密传输，防止中间人窃听

SQL 注入
现在用 mysql_real_escape_string 拼 SQL，能防一般注入。面试要说：“教学项目用 escape；生产更推荐 prepared statement，参数化查询，从根源上避免拼接。”
错误码设计

1000 缺参、1001/1002 校验、1003 JSON 非法、1004 用户名已存在、1005 用户不存在、1006 密码错误、5000 数据库不可用、5001 哈希失败。面试可以说：“按业务错误和系统错误分段，前端可以根据 code 做不同提示或降级。”

统一响应格式（工程化整理后）
{
  "code": 0,
  "msg": "ok",
  "data": {}
}

常见错误场景统一：
参数错误 -> HTTP 400
未登录/会话无效 -> HTTP 401
用户名已存在 -> HTTP 409
服务内部错误 -> HTTP 500

TTL = Time To Live（存活时间）
token：登录认证，用户登陆后生成 token，TTL 到期需要重新登录
Token 是 一种身份凭证（认证令牌）客户端之后的请求带上这个 token

登录
 ↓
服务器生成 token
 ↓
客户端保存 token
 ↓
请求携带 token
 ↓
服务器验证 token
---
Session：服务器保存的用户登录状态。
---
用户登录
   ↓
服务器创建 session
   ↓
返回 session_id
   ↓
浏览器保存 cookie
   ↓
后续请求携带 cookie
   ↓
服务器查 session

登录成功后会把 token 存进 Redis，同时设置过期时间。
当存 session 时用 SETEX key 1800 value，Redis 会在 1800 秒后自动删除这条 key。
用 Redis 的 TTL 做 session 自动过期，不用自己写定时任务清理，过期后 token 自然失效，符合常见的无状态 token + 有状态 session 的设计
---
bitmap 实现的布隆过滤器
---
服务器启动时，会先根据所有数据来初始化布隆过滤器，当收到登录请求时，会先根据布隆过滤器来判断该用户名是否一定不存在，如果能够判断不存在就不会查询 MySQL 数据库，减小开销。

1. 注册：
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"Mikey_","password":"pc520656"}'

2. 登录：
curl -X POST http://127.0.0.1:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","password":"pc520656"}'

3. 访问受保护接口（仅依赖 Redis session）：
curl http://127.0.0.1:8080/api/profile \
  -H "Authorization: Bearer <token>"

4. 登出（删除 Redis session）：
curl -X POST http://127.0.0.1:8080/api/logout \
  -H "Authorization: Bearer <token>"

5. 获取当前用户信息（受保护）：
curl http://127.0.0.1:8080/api/user/me \
  -H "Authorization: Bearer <token>"

6. 修改密码（受保护，成功后当前 token 失效）：
curl -X POST http://127.0.0.1:8080/api/user/password \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"old_password":"hello123","new_password":"hello456"}'

项目日志1：
在接入 Redis 做 session 存储时，程序启动阶段连接 Redis 异常，错误信息还是乱码。后续通过检查 ldd 和 pkg-config，定位到 hiredis 的头文件和运行时动态库版本不一致：编译走的是 /usr/local 的旧版 0.14.0，运行时加载的是系统 1.1.0。统一依赖版本后，Redis 连接恢复正常，session token 的写入与读取链路打通。

工程职责边界（小范围重构后）：
1) handler（`handlers_auth.cpp` / `handlers_user.cpp`）
- 只处理 HTTP 相关逻辑：解析请求、读取 header/token、调用 service、统一返回。
- 不直接编排数据库和 Redis 细节。

2) service（`auth_service.cpp` / `user_service.cpp`）
- 承接业务编排：参数业务校验、密码哈希/校验、session 生命周期动作、组合 repo 调用。
- 不处理 HTTP 协议细节（不依赖 Request/Response）。

3) repo（`mysql_user_repo.cpp` / `redis_session_repo.cpp`）
- 只负责数据访问和存储操作，不承载业务流程判断。

4) config（`config.cpp`）
- 只负责配置读取与注入（如 `bcrypt_cost`、`token_ttl_seconds`），由 `main.cpp` 传入 handler/service。
