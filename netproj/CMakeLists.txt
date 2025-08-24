cmake_minimum_required(VERSION 3.16)
project(NetProj CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_subdirectory(gen-app)
add_subdirectory(flowtop)
