include(common_macros)

set(_package freetype)
set(_target freetype)
set (FREETYPE_ROOT_DIR "" CACHE PATH "Path to freetype root directory")

simake_additional_search_dirs(${_package} "freetype-")

find_package(${_package} REQUIRED)

if (freetype_FOUND)
  message(STATUS "${_package} library has been found:")
  simake_print_includes_info(${_package} ${_target})
  if(TARGET ${_target})
    simake_set_library_dir_vars (${_target} FREETYPE_LIBRARY_DIRS FREETYPE_DLL_DIRS)
    simake_print_libs_info(${_package} FREETYPE_LIBRARY_DIRS FREETYPE_DLL_DIRS)
  endif()
endif()
