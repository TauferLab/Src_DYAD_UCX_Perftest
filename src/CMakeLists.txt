include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(backends)

set(libperftest_src
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/oob_comm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.c
)
set(libperftest_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/client.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/oob_comm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.h
)

add_library(perftest STATIC ${libperftest_src} ${libperftest_headers})
target_link_libraries(perftest PUBLIC backends CLI11::CLI11 nlohmann_json::nlohmann_json czmq caliper adiak::adiak MPI::MPI_C)
target_include_directories(perftest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/base64)

add_executable(dyad_ucx_perftest_server dyad_ucx_perftest_server.cpp)
target_link_libraries(dyad_ucx_perftest_server perftest caliper adiak::adiak)
add_executable(dyad_ucx_perftest_client dyad_ucx_perftest_client.cpp)
target_link_libraries(dyad_ucx_perftest_client perftest caliper adiak::adiak MPI::MPI_C)

install(
    TARGETS dyad_ucx_perftest_server dyad_ucx_perftest_client
    RUNTIME DESTINATION bin
)