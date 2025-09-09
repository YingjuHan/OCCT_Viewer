include(common_macros)

set (RapidJSON_ROOT_DIR "" CACHE PATH "Path to RapidJSON root directory")

simake_additional_search_dirs (RapidJSON "rapidjson-")

find_package(RapidJSON QUIET NO_CMAKE_SYSTEM_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_PACKAGE_REGISTRY CMAKE_FIND_ROOT_PATH_BOTH)

if (NOT ${RapidJSON_FOUND})
    find_path(_rapidjson_include_dirs NAMES rapidjson/rapidjson.h PATHS ${RapidJSON_ROOT_DIR} PATH_SUFFIXES include CMAKE_FIND_ROOT_PATH_BOTH)
endif()

if (NOT TARGET rapidjson)
    if (RAPIDJSON_INCLUDE_DIRS)
        set(_rapidjson_include_dirs ${RAPIDJSON_INCLUDE_DIRS})
    elseif(RAPIDJSON_INCLUDE_DIR)
        set(_rapidjson_include_dirs ${RAPIDJSON_INCLUDE_DIR})
    endif()

    if (_rapidjson_include_dirs)
        add_library(rapidjson INTERFACE IMPORTED)
        set_property(TARGET rapidjson PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${_rapidjson_include_dirs})
    endif()
endif()

if (TARGET rapidjson)
    get_property(_includes TARGET rapidjson PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    message("rapidjson include dirs: ${_includes}")
    set(RapidJSON_FOUND TRUE)
    if (NOT RAPIDJSON_INCLUDE_DIRS)
        set (RAPIDJSON_INCLUDE_DIRS ${_includes})
    endif()
else()
    message(FATAL_ERROR "RapidJSON not found")
endif()
