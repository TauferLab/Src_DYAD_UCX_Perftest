cmake_minimum_required(VERSION 3.16)

project(dyad_ucx_perftest LANGUAGES C CXX)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

option(ENABLE_LOG "" OFF)

if (ENABLE_LOG)
    add_definitions("-DENABLE_LOG=1")
    set(CMAKE_C_FLAGS "-g ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS}")
endif (ENABLE_LOG)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MPI REQUIRED)
find_package(ucx REQUIRED)
find_package(caliper REQUIRED)
find_package(adiak REQUIRED)

include(perftest_optimizations.cmake)

add_subdirectory(extern)
add_subdirectory(src)