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

# Define options for the different optimizations
option(OPT1 "Enable optimization 1 (UCP Memory Allocation)" OFF)
option(OPT2 "Enable optimization 1 (UCP Memory Allocation) and 2 (warmup iteration)" OFF)

# Add the necessary defines based on the values of the options
if (OPT1)
    add_definitions("-DOPT_1=1")
endif (OPT1)
if (OPT2)
    add_definitions("-DOPT_2=1")
endif (OPT2)

add_subdirectory(extern)
add_subdirectory(src)