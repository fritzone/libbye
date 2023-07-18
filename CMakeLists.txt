cmake_minimum_required(VERSION 3.5)

project(libby)

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
