set(backend_src
    ${CMAKE_CURRENT_SOURCE_DIR}/abstract_backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_backend/tag_backend.cpp
)
set(backend_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/abstract_backend.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_backend/tag_backend.hpp
)
set(backend_inc_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tag_backend
)

add_library(backends STATIC ${backend_src} ${backend_headers})
target_link_libraries(backends PUBLIC ucx::ucp ucx::ucs fmt::fmt caliper adiak::adiak)
target_include_directories(backends PUBLIC ${backend_inc_dirs})