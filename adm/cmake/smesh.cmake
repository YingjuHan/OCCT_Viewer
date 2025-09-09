include(common_macros)

set (SMESH_ROOT_DIR "${CONAN_SMESH_ROOT}" CACHE PATH "Path to SMESH root directory")

simake_additional_search_dirs (SMESH "smesh-")

find_package(SMESH REQUIRED PATH_SUFFIXES cmake)
