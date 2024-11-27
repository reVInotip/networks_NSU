cmake_minimum_required(VERSION 3.5)

project(socks_proxy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(dns-resolver/async_dns_resolver)
#include_directories(cpp_modules/cpp-dns)
include_directories(include dns-resolver/async_dns_resolver/include)
set(SOURCE_EXE main.cpp)
set(SOURCE_LIB_SOCKS_SERVER socks_server.cpp tunnel.cpp socket.cpp)

add_library(socks_server STATIC ${SOURCE_LIB_SOCKS_SERVER})
add_executable(${PROJECT_NAME} ${SOURCE_EXE})

target_link_libraries(${PROJECT_NAME} PRIVATE socks_server dns_resolve::lib)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -O2 -g -std=c++20
)