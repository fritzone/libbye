cmake_minimum_required(VERSION 3.27)

project(fpaq0)

set(${PROJECT_NAME}_SOURCES

    ${CMAKE_CURRENT_SOURCE_DIR}/fpaq0.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fpaq0.cpp
   )

add_library(${PROJECT_NAME}
    STATIC
    ${${PROJECT_NAME}_SOURCES}
)
