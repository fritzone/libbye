cmake_minimum_required(VERSION 3.27)
project(embed-resource)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

function(embed_resources out_var)
    set(result)
    foreach(in_f ${ARGN})
        file(RELATIVE_PATH src_f ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/${in_f}")
        set(out_f "${PROJECT_BINARY_DIR}/${in_f}.c")
        get_filename_component(out_f_dir "${out_f}" DIRECTORY)
        add_custom_command(OUTPUT ${out_f}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${out_f_dir}"
                COMMAND embed-resource "${out_f}" "${src_f}"
                DEPENDS "${in_f}" embed-resource
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Building binary file for embedding ${out_f}"
                VERBATIM)
        list(APPEND result "${out_f}")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

find_package(Boost COMPONENTS program_options filesystem REQUIRED )

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/embedresource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource.h
)

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBBYE_SOURCE_DIR}/ext/compressor)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${Boost_LIBRARIES}
    fpaq0
)

set(EMBED_RES_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
