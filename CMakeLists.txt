cmake_minimum_required(VERSION 3.10)
project(safe_login_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源文件
set(SOURCES
    src/main.cpp
    src/auth/handlers_auth.cpp
    src/auth/auth_service.cpp
    src/auth/http_auth.cpp
    src/user/handlers_user.cpp
    src/user/user_service.cpp
    src/repo/mysql_user_repo.cpp
    src/repo/redis_session_repo.cpp
    src/config/config.cpp
    src/api_response.cpp
    src/password_hash.cpp
)

add_executable(safe_login_server ${SOURCES})

target_include_directories(safe_login_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib
)

# pthread（httplib 的线程池等会用到）
find_package(Threads REQUIRED)
target_link_libraries(safe_login_server PRIVATE 
    Threads::Threads
    mysqlclient
    crypt
    hiredis
)
