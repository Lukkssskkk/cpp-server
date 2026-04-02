cmake_minimum_required(VERSION 3.10)

project(server-cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server-cpp src/main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(server-cpp PRIVATE Threads::Threads)