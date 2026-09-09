cmake_minimum_required(VERSION 3.20)
project(lq_c_api_demo LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lq_c_api_demo main.c)
target_include_directories(lq_c_api_demo PRIVATE ../../include)
target_link_libraries(lq_c_api_demo PRIVATE lq m)
