# VSCode configuration

if (NOT DEFINED VSCODE_WORKSPACE_DIR)
  set (VSCODE_WORKSPACE_DIR ${CMAKE_BINARY_DIR})
endif()

# Put a VSCode workspace file at the top of the build tree
set (VSCODE_WORKSPACE_FILE "${VSCODE_WORKSPACE_DIR}/occt-viewer-${CMAKE_BUILD_TYPE}.code-workspace")

include (ProcessorCount)
ProcessorCount (VSCODE_PROC_COUNT)
if (NOT VSCODE_PROC_COUNT)
  # expect at least 6 logical cores to be available on a build machine
  set (VSCODE_PROC_COUNT 6)
endif ()

if (NOT EXISTS "${VSCODE_WORKSPACE_FILE}")
  configure_file ("${CMAKE_SOURCE_DIR}/adm/templates/occt-viewer.code-workspace.in" "${VSCODE_WORKSPACE_FILE}")
  message (STATUS "Info: VSCode workspace file is stored to ${VSCODE_WORKSPACE_FILE}")
endif ()

# Put launch.json inside .vscode subdirectory to specify the executable and environment for debugging
set (VSCODE_CONFIG_DIR "${VSCODE_WORKSPACE_DIR}/.vscode")
if (NOT EXISTS "${VSCODE_CONFIG_DIR}")
  file (MAKE_DIRECTORY "${VSCODE_CONFIG_DIR}")
endif ()

set (VSCODE_LAUNCH_FILE "${VSCODE_CONFIG_DIR}/launch.json")

# Auxiliary development environment
if (NOT EXISTS "${VSCODE_LAUNCH_FILE}")
  if (WIN32)
    set (VSCODE_LAUNCH_ENV_VAR "PATH")
    set (VSCODE_LAUNCH_PATH "\${env:PATH}")
  else()
    set (VSCODE_LAUNCH_ENV_VAR "LD_LIBRARY_PATH")
    set (VSCODE_LAUNCH_PATH "\${env:LD_LIBRARY_PATH}")
  endif()

  # Binary path and environment for launching DRAWEXE from the build tree
  string (TOUPPER ${CMAKE_BUILD_TYPE} _conf_uc)
  set (VSCODE_LAUNCH_EXECUTABLE "${OpenCASCADE_BINARY_DIR}/DRAWEXE${CMAKE_EXECUTABLE_SUFFIX}")
  set (VSCODE_LAUNCH_PATH "${3RDPARTY_DLL_DIRS_${_conf_uc}};${3RDPARTY_DLL_DIRS};${VSCODE_LAUNCH_PATH}")
  set (VSCODE_LAUNCH_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY};${VSCODE_LAUNCH_PATH}")
  list (FILTER VSCODE_LAUNCH_PATH EXCLUDE REGEX "^$")

  set (VSCODE_DEBUGGER "")
  get_filename_component (VSCODE_DEBUGGER_PATH "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component (_COMPILER "${CMAKE_CXX_COMPILER}" NAME_WLE)
  get_filename_component (_EXT "${CMAKE_CXX_COMPILER}" LAST_EXT)
  if ("${_COMPILER}" STREQUAL "g++" OR "${_COMPILER}" STREQUAL "c++")
    set (VSCODE_DEBUGGER "${VSCODE_DEBUGGER_PATH}/gdb${_EXT}")
  endif()

  configure_file ("${CMAKE_SOURCE_DIR}/adm/templates/launch.json.in" "${VSCODE_LAUNCH_FILE}" @ONLY)
  message (STATUS "Info: VSCode configuration is stored to ${VSCODE_LAUNCH_FILE}")
endif()