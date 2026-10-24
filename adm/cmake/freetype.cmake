include(common_macros)

set(_package freetype)
set(_target freetype)
set (FREETYPE_ROOT_DIR "" CACHE PATH "Path to freetype root directory")

if (FREETYPE_ROOT_DIR AND NOT DEFINED ENV{FREETYPE_DIR})
  set(ENV{FREETYPE_DIR} "${FREETYPE_ROOT_DIR}")
endif()

simake_additional_search_dirs(${_package} "freetype-")

find_package(${_package} REQUIRED)

if (NOT TARGET ${_target})
  if (TARGET Freetype::Freetype)
    add_library(${_target} ALIAS Freetype::Freetype)
  elseif (FREETYPE_LIBRARY OR FREETYPE_LIBRARIES)
    set(_freetype_lib "${FREETYPE_LIBRARY}")
    if (NOT _freetype_lib AND FREETYPE_LIBRARIES)
      list(GET FREETYPE_LIBRARIES 0 _freetype_lib)
    endif()
    if (_freetype_lib AND FREETYPE_INCLUDE_DIRS)
      add_library(${_target} STATIC IMPORTED)
      set_target_properties(${_target} PROPERTIES
        IMPORTED_LOCATION "${_freetype_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}")
    endif()
  endif()
endif()

if (freetype_FOUND)
  message(STATUS "${_package} library has been found:")
  simake_print_includes_info(${_package} ${_target})
  if(TARGET ${_target})
    simake_set_library_dir_vars (${_target} FREETYPE_LIBRARY_DIRS FREETYPE_DLL_DIRS)
    simake_print_libs_info(${_package} FREETYPE_LIBRARY_DIRS FREETYPE_DLL_DIRS)
  endif()
endif()
