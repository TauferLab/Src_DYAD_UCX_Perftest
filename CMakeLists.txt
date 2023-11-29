cmake_minimum_required(VERSION 3.16)

project(dyad_ucx_perftest LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ucx REQUIRED)
find_package(caliper REQUIRED)
find_package(adiak REQUIRED)

add_subdirectory(extern)
add_subdirectory(src)