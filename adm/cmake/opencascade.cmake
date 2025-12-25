include(common_macros)

set(OpenCASCADE_ROOT_DIR "" CACHE PATH "Path to OpenCASCADE root directory")

simake_additional_search_dirs (OpenCASCADE "opencascade-")

if (OpenCASCADE_ROOT_DIR AND NOT OpenCASCADE_DIR)
  # Help CMake find OpenCASCADEConfig.cmake when only installation prefix is provided.
  set (OpenCASCADE_DIR "${OpenCASCADE_ROOT_DIR}/lib/cmake/opencascade" CACHE PATH "Path to OpenCASCADE CMake config directory" FORCE)
endif()

find_package(OpenCASCADE QUIET PATH_SUFFIXES lib/cmake/opencascade)

if (NOT OpenCASCADE_FOUND)
    include(opencascade_fallback)
endif()

if (OpenCASCADE_FOUND)
    message(STATUS "OpenCASCADE library has been found: ${OpenCASCADE_INSTALL_PREFIX}")
    message(STATUS "OpenCASCADE include directories: ${OpenCASCADE_INCLUDE_DIR}")
    message(STATUS "OpenCASCADE library directories: ${OpenCASCADE_LIBRARY_DIR}")
    message(STATUS "OpenCASCADE binary: ${OpenCASCADE_BINARY_DIR}")

    if (WIN32)
        simake_collect_dll_paths(3RDPARTY_DLL_DIRS OpenCASCADE_BINARY_DIR)
    else()
        simake_collect_dll_paths(3RDPARTY_DLL_DIRS OpenCASCADE_LIBRARY_DIR)
    endif()

    add_executable(DRAWEXE IMPORTED)
    set(draw_exe "${OpenCASCADE_BINARY_DIR}/DRAWEXE${CMAKE_EXECUTABLE_SUFFIX}")
    set(draw_exe_debug "${OpenCASCADE_BINARY_DIR}/dDRAWEXE${CMAKE_EXECUTABLE_SUFFIX}")

    if (EXISTS ${draw_exe})
        message(STATUS "OpenCASCADE draw executable: ${draw_exe}")
        set_target_properties(DRAWEXE PROPERTIES IMPORTED_LOCATION ${draw_exe})
        set_target_properties(DRAWEXE PROPERTIES IMPORTED_LOCATION_RELEASE ${draw_exe})
    endif()
    if (EXISTS ${draw_exe_debug})
        message(STATUS "OpenCASCADE draw executable debug: ${draw_exe_debug}")
        set_target_properties(DRAWEXE PROPERTIES IMPORTED_LOCATION_DEBUG ${draw_exe_debug})

        if (NOT EXISTS ${draw_exe})
            set_target_properties(DRAWEXE PROPERTIES IMPORTED_LOCATION ${draw_exe_debug})
        endif()
    endif()
else()
    message(FATAL_ERROR "OpenCASCADE library not found")
endif()
