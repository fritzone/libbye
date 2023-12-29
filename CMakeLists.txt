cmake_minimum_required(VERSION 3.5)

project(libby)

cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0002 NEW)
cmake_policy(SET CMP0135 NEW)

# These two below have a role to play when building a project which has this as a "subproject"
set(LIBBYE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "" FORCE)
set(LIBBYE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "" FORCE)

# Inja uses nlohmann json too, disable it
set(INJA_USE_EMBEDDED_JSON OFF CACHE STRING "OFF" FORCE)
set(INJA_BUILD_TESTS OFF CACHE STRING "OFF" FORCE)
set(BUILD_BENCHMARK OFF CACHE STRING "OFF" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# JSON library
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

add_subdirectory(ext)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext/inja/include)
add_subdirectory(parse)

add_subdirectory(testlib)
add_subdirectory(testapp)

include(cmake/Embed.cmake)
Embed(testlib testapp)
