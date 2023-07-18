#
# Embeds the given LIBRARY into the given APPLICATION
#
function (Embed LIBRARY APPLICATION)

    get_target_property(${LIBRARY}_SOURCES ${LIBRARY} SOURCES)
    get_target_property(${LIBRARY}_CFLAGS ${LIBRARY} COMPILE_OPTIONS)

    get_target_property(${LIBRARY}_DIR ${LIBRARY} SOURCE_DIR)
    get_target_property(${LIBRARY}_LIB_NAME ${LIBRARY} LIBRARY_OUTPUT_NAME)
    message("Target: ${${LIBRARY}_LIB_NAME}")

    get_target_property(${LIBRARY}_DIRS ${LIBRARY} INCLUDE_DIRECTORIES)

    message("DIRS:${${LIBRARY}_DIRS}")
    foreach(dir ${${LIBRARY}_DIRS})
      message(STATUS "dir='${dir}'")
    endforeach()

    set(LIBRARY_FILES "")

    foreach(LIBRARY_SOURCE_FILE ${${LIBRARY}_SOURCES})
        message( ${${LIBRARY}_DIR} " - " ${LIBRARY_SOURCE_FILE})

        list(APPEND LIBRARY_FILES "${${LIBRARY}_DIR}/${LIBRARY_SOURCE_FILE}")
    endforeach()

    add_custom_command(
        OUTPUT ${LIBRARY}_FUNS_HEADER
        DEPENDS parser ${LIBRARY}
        COMMAND ${CMAKE_BINARY_DIR}/parse/parser
                --library $<TARGET_FILE_DIR:${LIBRARY}>/$<TARGET_FILE_NAME:${LIBRARY}>
                --libname ${LIBRARY}
                --proxy_header ${CMAKE_SOURCE_DIR}/parse/templates/header.h
                --target_dir ${CMAKE_BINARY_DIR}/proxies/$<TARGET_FILE_DIR:${APPLICATION}>/${LIBRARY}
                --compilationdb ${CMAKE_BINARY_DIR}/compile_commands.json
                ${LIBRARY_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_target(
        ${LIBRARY}_COMPILE ALL
        DEPENDS ${LIBRARY}_FUNS_HEADER
    )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}_resource.h
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}_resource.h
        DEPENDS embed-resource ${LIBRARY}
        COMMAND $<TARGET_FILE_DIR:embed-resource>/$<TARGET_FILE_NAME:embed-resource>
                ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}_resource.h  $<TARGET_FILE_DIR:${LIBRARY}>/$<TARGET_FILE_NAME:${LIBRARY}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )

add_library(bar OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}_resource.h)

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}_resource.h PROPERTIES GENERATED 1)


    add_custom_target(
        ${LIBRARY}_RESOURCEIZER ALL
        DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}_resource.h
    )

    add_dependencies(${APPLICATION} ${LIBRARY}_RESOURCEIZER)

    target_sources( ${APPLICATION}
        PRIVATE
            $<TARGET_OBJECTS:bar>
    )

    target_include_directories(${APPLICATION} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

endfunction()
