cmake_minimum_required(VERSION 3.10)
project(safe_login_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源文件
set(SOURCES
    src/main.cpp
    src/api_response.cpp
    src/auth_service.cpp
    src/config.cpp
    src/handlers_auth.cpp
    src/handlers_user.cpp
    src/http_auth.cpp
    src/mysql_user_repo.cpp
    src/password_hash.cpp
    src/redis_session_repo.cpp
    src/user_service.cpp
)

add_executable(safe_login_server ${SOURCES})

target_include_directories(safe_login_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
