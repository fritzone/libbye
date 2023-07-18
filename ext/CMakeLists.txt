cmake_minimum_required(VERSION 3.5)

set(BUILD_BENCHMARK OFF)
set(INJA_USE_EMBEDDED_JSON OFF)
set(INJA_BUILD_TESTS OFF)
add_subdirectory(inja)

include_directories(embed-resource)
add_subdirectory(embed-resource)
