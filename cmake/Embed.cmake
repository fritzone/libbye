#
# Embeds the given LIBRARY into the given APPLICATION
#
function (Embed LIBRARY APPLICATION)

    message("libbye dir:" ${LIBBYE_SOURCE_DIR} " libbye build dir:" ${LIBBYE_BINARY_DIR})

    cmake_policy(SET CMP0079 NEW)

    # Various initialization stuff
    get_target_property(${LIBRARY}_SOURCES ${LIBRARY} SOURCES)
    get_target_property(${LIBRARY}_CFLAGS ${LIBRARY} COMPILE_OPTIONS)
    get_target_property(${LIBRARY}_DIR ${LIBRARY} SOURCE_DIR)
    get_target_property(${LIBRARY}_LIB_NAME ${LIBRARY} LIBRARY_OUTPUT_NAME)
    get_target_property(${LIBRARY}_DIRS ${LIBRARY} INCLUDE_DIRECTORIES)

    # This will contain the source files of the given library
    set(LIBRARY_FILES "")

    foreach(LIBRARY_SOURCE_FILE ${${LIBRARY}_SOURCES})
        message(STATUS "Consolidating source " ${${LIBRARY}_DIR} "/" ${LIBRARY_SOURCE_FILE})
        list(APPEND LIBRARY_FILES "${${LIBRARY}_DIR}/${LIBRARY_SOURCE_FILE}")
    endforeach()

    add_custom_command(
        OUTPUT ${LIBRARY}_FUNS_HEADER
        DEPENDS parser ${LIBRARY}
        COMMAND ${LIBBYE_BINARY_DIR}/parse/parser
                --library $<TARGET_FILE_DIR:${LIBRARY}>/$<TARGET_FILE_NAME:${LIBRARY}>
                --libname ${LIBRARY}
                --proxy_header ${LIBBYE_SOURCE_DIR}/parse/templates/proxy_header_template.h
                --proxy_source ${LIBBYE_SOURCE_DIR}/parse/templates/proxy_source_template.cpp
                --target_dir ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/
                --compilationdb ${CMAKE_BINARY_DIR}/compile_commands.json
                ${LIBRARY_FILES}
        WORKING_DIRECTORY ${LIBBYE_BINARY_DIR}
        COMMENT "Generating proxy implementation for $<TARGET_FILE_NAME:${LIBRARY}>"
    )

    # A custom target to compile the proxy code
    add_custom_target(
        ${LIBRARY}_COMPILE ALL
        DEPENDS ${LIBRARY}_FUNS_HEADER
    )

    add_dependencies(${APPLICATION} ${LIBRARY}_COMPILE)

    # Will generate the resource header
    add_custom_command(
        OUTPUT ${LIBBYE_BINARY_DIR}/${LIBRARY}_resource.h
        BYPRODUCTS ${LIBBYE_BINARY_DIR}/${LIBRARY}_resource.h
        DEPENDS embed-resource ${LIBRARY}
        COMMAND $<TARGET_FILE_DIR:embed-resource>/$<TARGET_FILE_NAME:embed-resource>
                --symbol ${LIBRARY}
                --output ${LIBBYE_BINARY_DIR}/${LIBRARY}_resource.h
                --resource $<TARGET_FILE_DIR:${LIBRARY}>/$<TARGET_FILE_NAME:${LIBRARY}>
        WORKING_DIRECTORY ${LIBBYE_BINARY_DIR}
        VERBATIM
    )

    add_library(${APPLICATION}_resource OBJECT ${LIBBYE_BINARY_DIR}/${LIBRARY}_resource.h)

    set_source_files_properties(${LIBBYE_BINARY_DIR}/${LIBRARY}_resource.h PROPERTIES GENERATED 1)

    add_custom_target(
        ${LIBRARY}_RESOURCEIZER ALL
        DEPENDS  ${LIBBYE_BINARY_DIR}/${LIBRARY}_resource.h
    )

    add_dependencies(${APPLICATION} ${LIBRARY}_RESOURCEIZER)

    target_sources( ${APPLICATION}
        PRIVATE
            $<TARGET_OBJECTS:${APPLICATION}_resource>
    )

    add_library(${APPLICATION}_proxies OBJECT
        ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/${LIBRARY}_proxy_header.h
        ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/${LIBRARY}_proxy_source.cpp
    )

    set_source_files_properties(${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/${LIBRARY}_proxy_header.h PROPERTIES GENERATED 1)
    set_source_files_properties(${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/${LIBRARY}_proxy_source.cpp PROPERTIES GENERATED 1)
    add_custom_target(
        ${LIBRARY}_proxyizer ALL
        DEPENDS ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/${LIBRARY}_proxy_header.h
        ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/${LIBRARY}_proxy_source.cpp
    )
    target_sources( ${APPLICATION}
        PRIVATE
            $<TARGET_OBJECTS:${APPLICATION}_proxies>
    )

    add_dependencies(${APPLICATION} ${LIBRARY}_proxyizer ${LIBRARY}_RESOURCEIZER)

    include_directories(
        ${LIBBYE_BINARY_DIR}
        ${LIBBYE_SOURCE_DIR}/ext/embed-resource
        ${LIBBYE_SOURCE_DIR}/ext/compressor
        ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/
    )

    target_include_directories(${APPLICATION} PRIVATE
        ${LIBBYE_BINARY_DIR}
        ${LIBBYE_SOURCE_DIR}/ext/embed-resource
        ${LIBBYE_SOURCE_DIR}/ext/compressor
        ${LIBBYE_BINARY_DIR}/proxies/${APPLICATION}/${LIBRARY}/
    )

    target_link_libraries(${APPLICATION} PRIVATE fpaq0)

endfunction()
