# Draco detection procedure is the following:
# By default, CMake looks for configuration files in a folder specified by <Package>_DIR variable, i.e draco_DIR.
# Also a location of libraries can be defined via
#  2. draco_ROOT_DIR CMake variables (it should point to Draco installation directory)
#  3. 3RDPARTY_DIR CMake of environment variable. In this case draco libraries are searched in the
#     <3RDPARTY_DIR>/draco-* directory.

include(common_macros)
set(_draco_package draco)
set(_draco_target draco::draco)

# Looking for draco additional search directories
simake_additional_search_dirs (${_draco_package} "draco-")
# Find draco library
if (NOT TARGET ${_draco_target})
  find_package (${_draco_package} REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
else()
  set(draco_FOUND ON)
endif()

# Find draco library
if (draco_FOUND)
  if (TARGET ${_draco_target})
    message (STATUS "${_draco_package} library has been found:")
    simake_print_includes_info(${_draco_package} ${_draco_target})
    simake_set_library_dir_vars (${_draco_target} DRACO_LIBRARY_DIRS DRACO_DLL_DIRS)
    simake_print_libs_info(${_draco_package} DRACO_LIBRARY_DIRS DRACO_DLL_DIRS)
  endif()
endif()