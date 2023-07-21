cmake_minimum_required(VERSION 3.19)

project(parser LANGUAGES CXX)

find_package(Boost COMPONENTS program_options filesystem REQUIRED )

add_executable(parser main.cpp)

install(TARGETS parser
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(parser
    clang-14
    nlohmann_json::nlohmann_json
    ${Boost_LIBRARIES}
    inja
)
