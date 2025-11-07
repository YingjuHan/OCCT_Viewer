# This CMake module contains common macros and functions

# Macro to find sub-directory by mask
#  - result: full path to found directory
#  - rootdir: root directory to search
#  - masks list of masks to search
macro (simake_find_subdir_by_mask result rootdir masks)
  file (GLOB subdirs LIST_DIRECTORIES true RELATIVE ${rootdir} ${rootdir}/*)
  set (${result})
  foreach (subdir ${subdirs})
    if (IS_DIRECTORY ${rootdir}/${subdir})
      foreach (mask ${masks})
        string (REGEX MATCH ${mask} founddir ${subdir})
        if (founddir)
          set (${result} ${rootdir}/${subdir})
          break()
        endif()
      endforeach()
      if (${result})
        break()
      endif()
    endif()
  endforeach()
endmacro()

# Macro to add additional search directories for third-party dependencies
# The procedure for specifying additional directories to search is the following:
#  1. If <PACKAGE_NAME>_ROOT_DIR is set, it is added to CMAKE_PREFIX_PATH
#  2. Overwise if 3RDPARTY_DIR CMake or 3RDPARTY_DIR environment variable is set,
#     try to find third-party product directory by masks, specified with <masks> parameter.
#  3. If 3RDPARTY_DIR or <PACKAGE_NAME>_ROOT_DIR variables are not set, try to find
#     third-party libraries relatively sources directory /../../3rdparty/<package_name>-X.Y.Z and etc...
macro (simake_additional_search_dirs package_name masks)
  string (TOUPPER ${package_name} _package_name_uc)
  if (${_package_name_uc}_ROOT_DIR OR ${package_name}_ROOT_DIR)
    set (_${package_name}_root_dir ${${_package_name_uc}_ROOT_DIR})
    if (NOT _${package_name}_root_dir)
      set (_${package_name}_root_dir ${${package_name}_ROOT_DIR})
    endif()
  else()
    # Add variable into cache
    set (3RDPARTY_DIR "" CACHE PATH "Path to third-party libraries")
    # 3RDPARTY_DIR variable is defined
    if (3RDPARTY_DIR)
      set (_3rd_party_dir ${3RDPARTY_DIR})
    elseif (DEFINED ENV{3RDPARTY_DIR})
      set (_3rd_party_dir ENV{3RDPARTY_DIR})
    endif()
    if (_3rd_party_dir)
      message (STATUS "3rd-party products directory is ${3RDPARTY_DIR}")
      set (3RDPARTY_DIR ${_3rd_party_dir} CACHE PATH "3rd-party products directory" FORCE)
    endif()
    if (3RDPARTY_DIR)
      simake_find_subdir_by_mask (_${package_name}_found_root ${3RDPARTY_DIR} ${masks})
      if (_${package_name}_found_root)
        set (_${package_name}_root_dir ${_${package_name}_found_root})
      endif()
    endif()
  endif()
  if (_${package_name}_root_dir AND EXISTS ${_${package_name}_root_dir})
    list(APPEND CMAKE_PREFIX_PATH ${_${package_name}_root_dir})
  endif()
endmacro()

# Function to unset cache variables by mask <mask>
function (simake_clear_cache_variables mask)
  get_cmake_property (_variable_names CACHE_VARIABLES)
  foreach (_variable_name ${_variable_names})
    unset(_matched)
    if (mask)
      string (REGEX MATCH ${mask} _matched ${_variable_name})
      if (NOT _matched)
        continue()
      endif()
      unset (${_variable_name} CACHE)
    endif()
  endforeach()
endfunction()

# Macro to retrieve and activate conan configuration.
# This macro performs 'conan install <path_to_conan_file>' command in the ${CMAKE_BINARY_DIR} directory
# and activates retrieved conan CMake configuration.
# Optional parameter CONANFILE can be used to specify a path to conan file,
# by default ${CMAKE_SOURCE_DIR}/conanfile.txt path is used.
macro (simake_conan_install)
  if (NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    cmake_parse_arguments (CONAN_INSTALL "" "CONANFILE" "" ${ARGN})
    if (CONAN_INSTALL_CONANFILE)
      set (_conanfile ${CONAN_INSTALL_CONANFILE})
    else()
      set (_conanfile ${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt)
    endif()
    # Try to find conan.cmake file into Conan home directory
    execute_process(COMMAND conan config home
                    RESULT_VARIABLE _command_return_code
                    OUTPUT_VARIABLE _command_output
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set (_conan_file_path "${_command_output}/cmake/conan.cmake")
    if (NOT (${_command_return_code} EQUAL 0 AND EXISTS ${_conan_file_path}))
      set (_conan_file_url "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake")
      file (DOWNLOAD ${_conan_file_url}
            ${CMAKE_BINARY_DIR}/conan.cmake STATUS _download_status_list LOG _download_log)
      list (GET _download_status_list 0 _download_status)
      if (NOT ${_download_status} EQUAL 0)
        message (ERROR ${_download_log})
        message (FATAL_ERROR "Can't download file '${_conan_file_url}'")
      endif()
      set (_conan_file_path ${CMAKE_BINARY_DIR}/conan.cmake)
    endif()
    include (${_conan_file_path})
    conan_cmake_run (CONANFILE ${_conanfile} UPDATE)
  endif()
  if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include (${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup (NO_OUTPUT_DIRS)
  endif()
endmacro()

# Macro to install all third-party dependencies (except system).
# This macro iterates by all sub-folders (added with add_subfolder(...) command)
# starting from <root_folder> and collects all targets from each sub-folder.
# After that for each found target all dependencies are detected using
# file(GET_RUNTIME_DEPENDENCIES ...) or get_prerequisites (from GetPrerequisites.cmake) command, it depends on
# DEPENDENCIES_TOOL parameter ('file' or 'get_prerequisites')
#
# Mandatory parameter DESTINATION is used to specify destination folder for dependencies install:
# these folders can be specified in the following ways:
#  1. Using variables for each configuration (Release, Debug, RelWithDebInfo, etc..), for example:
#     the following call:
#        set(PROJECT_INSTALL_LIB_RELEASE lib)
#        set(PROJECT_INSTALL_LIB_DEBUG   libd)
#        simake_install_dependencies(${CMAKE_SOURCE_DIR} DESTINATION PROJECT_INSTALL_LIB)
#     installs dependencies for
#          * Release configuration into ${CMAKE_INSTALL_PREFIX}/lib
#          * Debug configuration into ${CMAKE_INSTALL_PREFIX}/libd
#  2. Using full path to the destination directory, for example the following call:
#        simake_install_dependencies(${CMAKE_SOURCE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
#     installs dependencies into ${CMAKE_INSTALL_PREFIX}/lib for all configurations.
#
# Optional parameter EXECUTABLES_DESTINATION is used to specify destination folder for executables,
# specified with ADDITIONAL_DEPENDENCIES parameter
# Optional parameter ADDITIONAL_DEPENDENCIES is used to specify additional executables targets.
# Optional parameter ADDITIONAL_DIRECTORIES is used to specify additional search directories.
# Optional parameter ADDITIONAL_FILES is used to specify additional files to install.
# Optional parameter EXCLUDE_LIBRARIES_REGEXP is used to specify regex for libraries names, to exclude from install.
# Finally all found dependencies are installed into DESTINATION directory.
macro (simake_install_dependencies root_folder)
  cmake_parse_arguments (INSTALL_DEP "" "DESTINATION;DEPENDENCIES_TOOL"
                         "ADDITIONAL_DEPENDENCIES;ADDITIONAL_DIRECTORIES;ADDITIONAL_FILES;EXECUTABLES_DESTINATION;EXCLUDE_LIBRARIES_REGEXP"
                         ${ARGN})
  set (_dependencies_tool get_prerequisites)
  set (_system_paths_regexp "^(/lib/|/lib32/|/libx32/|/lib64/|/usr/lib/|/usr/lib32/|/usr/libx32/|\
/usr/lib64/|/usr/X11R6/|/usr/bin/)" CACHE INTERNAL "Regular expression to detect system paths")
  if (INSTALL_DEP_DEPENDENCIES_TOOL)
    if (${INSTALL_DEP_DEPENDENCIES_TOOL} STREQUAL file OR
        ${INSTALL_DEP_DEPENDENCIES_TOOL} STREQUAL get_prerequisites)
        set (_dependencies_tool ${INSTALL_DEP_DEPENDENCIES_TOOL})
    else()
      message (SEND_ERROR "install_dependencies(...) bad 'DEPENDENCIES_TOOL' parameter's value \
${INSTALL_DEP_DEPENDENCIES_TOOL}, only 'file' or 'get_prerequisites' are allowed")
    endif()
  endif()
  if (NOT INSTALL_DEP_DESTINATION)
    message (SEND_ERROR "install_dependencies(...) mandatory parameter 'DESTINATION' is not defined")
  endif()
  simake_get_project_configurations (_configurations)
  # Iterate over all configurations
  foreach (_conf ${_configurations})
    string (TOUPPER ${_conf} _uc_conf)
    set (_all_third_party_targets_${_conf})
    set (_all_third_party_dirs_${_conf})
    get_property (_sub_dirs DIRECTORY ${root_folder} PROPERTY SUBDIRECTORIES)
    # Iterate over all sub-directories and collect all targets associated with each sub-directory
    foreach (_dir ${_sub_dirs})
      message (STATUS "Looking for targets in ${_dir} for ${_conf} configuration")
      get_property (_found_targets DIRECTORY ${_dir} PROPERTY BUILDSYSTEM_TARGETS)
      foreach (_target ${_found_targets})
        get_target_property (target_type ${_target} TYPE)
        if (${target_type} STREQUAL SHARED_LIBRARY OR ${target_type} STREQUAL EXECUTABLE OR
            ${target_type} STREQUAL MODULE_LIBRARY)
          if (${target_type} STREQUAL MODULE_LIBRARY)
            set(target_type SHARED_LIBRARY)
          endif()
          message (STATUS " * ${_target}")
          list (APPEND _all_targets_${target_type} "\$<TARGET_FILE:${_target}>")
          _get_target_dependencies (${_target} ${_conf} _all_third_party_targets_${_conf} _all_third_party_dirs_${_conf})
        endif()
      endforeach()
    endforeach()
    list (JOIN _all_third_party_dirs_${_conf} "\n" _file_content)
    if  (${INSTALL_DEP_DESTINATION}_${_uc_conf})
      set (target_dir_${_conf} ${CMAKE_INSTALL_PREFIX}/${${INSTALL_DEP_DESTINATION}_${_uc_conf}})
    else()
      set (target_dir_${_conf} ${INSTALL_DEP_DESTINATION})
    endif()
    if (${INSTALL_DEP_EXECUTABLES_DESTINATION}_${_uc_conf})
      set (exec_target_dir_${_conf} ${CMAKE_INSTALL_PREFIX}/${${INSTALL_DEP_EXECUTABLES_DESTINATION}_${_uc_conf}})
    else()
      if(INSTALL_DEP_EXECUTABLE_DESTINATION)
        set (exec_target_dir_${_conf} ${INSTALL_DEP_EXECUTABLES_DESTINATION}})
      endif()
    endif()
  endforeach()

  file (GENERATE OUTPUT ${CMAKE_BINARY_DIR}/_all_deps_$<CONFIG>.info CONTENT "${_file_content}")

  if (INSTALL_DEP_ADDITIONAL_DEPENDENCIES)
    message (STATUS "Process additional targets:")
    foreach (_target ${INSTALL_DEP_ADDITIONAL_DEPENDENCIES})
      if (TARGET ${_target})
        get_target_property (target_type ${_target} TYPE)
        if (${target_type} STREQUAL SHARED_LIBRARY OR ${target_type} STREQUAL EXECUTABLE OR
            ${target_type} STREQUAL MODULE_LIBRARY OR ${target_type} STREQUAL UNKNOWN_LIBRARY)
          if (${target_type} STREQUAL MODULE_LIBRARY OR ${target_type} STREQUAL UNKNOWN_LIBRARY)
            set( target_type SHARED_LIBRARY)
          endif()
          list (APPEND _all_additional_targets_${target_type} "\$<TARGET_FILE:${_target}>")
          message (STATUS " * ${_target}")
        endif()
      else()
        list (APPEND _all_additional_targets_${target_type} "${_target}")
        message (STATUS " * ${_target}")
      endif()
    endforeach()
  endif()
  list (JOIN _all_targets_EXECUTABLE "\n" _file_content_exec)
  file (GENERATE OUTPUT ${CMAKE_BINARY_DIR}/_exec_targets_$<CONFIG>.info CONTENT "${_file_content_exec}")
  list (JOIN _all_targets_SHARED_LIBRARY "\n" _file_content_libs)
  file (GENERATE OUTPUT ${CMAKE_BINARY_DIR}/_libs_targets_$<CONFIG>.info CONTENT "${_file_content_libs}")
  list (JOIN _all_additional_targets_EXECUTABLE "\n" _file_additional_content_exec)
  file (GENERATE OUTPUT ${CMAKE_BINARY_DIR}/_exec_targets_additional_$<CONFIG>.info CONTENT "${_file_additional_content_exec}")
  list (JOIN _all_additional_targets_SHARED_LIBRARY "\n" _file_additional_content_libs)
  file (GENERATE OUTPUT ${CMAKE_BINARY_DIR}/_libs_targets_additional_$<CONFIG>.info CONTENT "${_file_additional_content_libs}")

  install (CODE "set( _dependencies_tool \"${_dependencies_tool}\")")
  install (CODE "set( _additional_dirs \"${INSTALL_DEP_ADDITIONAL_DIRECTORIES}\")")
  if (WIN32)
    set (_win_root $ENV{WINDIR})
    if (_win_root)
      file (TO_CMAKE_PATH "${_win_root}" _win_root_cmake)
      foreach (_win_sys_dir "System32" "System32/downlevel" "SysWOW64")
        set (_win_sys_full "${_win_root_cmake}/${_win_sys_dir}")
        if (EXISTS "${_win_sys_full}")
          install (CODE "list(APPEND _additional_dirs \"${_win_sys_full}\")")
        endif()
      endforeach()
    endif()
  endif()
  install (CODE "set( _additional_files \"${INSTALL_DEP_ADDITIONAL_FILES}\")")
  if (INSTALL_DEP_EXCLUDE_LIBRARIES_REGEXP)
    install (CODE "set (_exclude_regexp \"${INSTALL_DEP_EXCLUDE_LIBRARIES_REGEXP}\")")
  endif()
  foreach (_conf ${_configurations} "")
    string (TOUPPER "${_conf}" _uc_conf)
    set(suff _)
    if (_conf MATCHES "^$")
      set (_suff "")
    endif()
    if(3RDPARTY_DLL_DIRS${_suff}${_uc_conf})
      install(CODE "set (3RDPARTY_DLL_DIRS${_suff}${_uc_conf} \"${3RDPARTY_DLL_DIRS${_suff}${_uc_conf}}\")")
    endif()
  endforeach()

  # Regular expression to exclude system directories
  if (WIN32)
    set (_win_root $ENV{WINDIR})
    if (_win_root)
      string(REPLACE "\\" "\\\\\\\\" _w_win_root ${_win_root})
      file (TO_CMAKE_PATH "${_win_root}" _u_win_root)
      install (CODE "set (_system_paths \"^(${_u_win_root}|${_w_win_root})\")")
    endif()
    if (MINGW)
      get_filename_component (_mingw_path ${CMAKE_CXX_COMPILER} PATH)
      install (CODE "set (_mingw_path \"${_mingw_path}\")")
    endif()
  else()
    install (CODE "set(_system_paths \"^(/lib/|/lib32/|/libx32/|/lib64/|/usr/lib/|/usr/lib32/|/usr/libx32/|\
/usr/lib64/|/usr/X11R6/|/usr/bin/)\")")
  endif()
  foreach (_cfg ${_configurations})
    install (CODE "set(target_dir_${_cfg} \"${target_dir_${_cfg}}\")")
    if(exec_target_dir_${_cfg})
      install (CODE "set(exec_target_dir_${_cfg} \"${exec_target_dir_${_cfg}}\")")
    endif()
  endforeach()

  install(CODE [[
    # Current install configuration:
    set (_conf ${CMAKE_INSTALL_CONFIG_NAME})
    string (TOUPPER ${_conf} _uc_conf)
    # Read files with list of dependencies
    file (STRINGS ${CMAKE_BINARY_DIR}/_libs_targets_${_conf}.info _libs_${_conf})
    file (STRINGS ${CMAKE_BINARY_DIR}/_exec_targets_additional_${_conf}.info _additional_execs_${_conf})
    file (STRINGS ${CMAKE_BINARY_DIR}/_libs_targets_additional_${_conf}.info _additional_libs_${_conf})
    file (STRINGS ${CMAKE_BINARY_DIR}/_all_deps_${_conf}.info _all_deps_${_conf})

    if(3RDPARTY_DLL_DIRS)
      list( APPEND _additional_dirs ${3RDPARTY_DLL_DIRS})
    endif()
    if(3RDPARTY_DLL_DIRS_${_uc_conf})
      list( APPEND _additional_dirs ${3RDPARTY_DLL_DIRS_${_uc_conf}})
    endif()

    set (_third_party_dirs)
    set (_deps)
    foreach (_item ${_all_deps_${_conf}} ${_additional_execs_${_conf}} ${_additional_libs_${_conf}})
      get_filename_component (_dir ${_item} DIRECTORY)
      if (_dir)
        list (FIND _third_party_dirs ${_dir} _idx)
        string (REGEX MATCH ${_system_paths} _path_match ${_dir})
        if ((NOT ${_idx} GREATER -1) AND (NOT _path_match))
          list (APPEND _third_party_dirs ${_dir})
        endif()
      endif()
    endforeach()
    # Get dependencies cmake built-in tools
    if (${_dependencies_tool} STREQUAL file)
      file (GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR _all_rt_deps_${_conf}
            UNRESOLVED_DEPENDENCIES_VAR _all_un_deps
            DIRECTORIES ${_additional_dirs} ${_third_party_dirs} ${_mingw_path}
            LIBRARIES ${_libs_${_conf}} ${_additional_libs_${_conf}}
            EXECUTABLES ${_execs_${_conf}} ${_additional_libs_${_conf}}
            POST_EXCLUDE_FILES ${_libs_${_conf}} ${_additional_libs_${_conf}} ${_execs_${_conf}} ${_additional_execs_${_conf}}
            POST_EXCLUDE_REGEXES ${_system_paths})
      list (LENGTH _all_un_deps _all_un_deps_length)
      if (${_all_un_deps_length} GREATER 0)
        message (FATAL_ERROR "Unresolved dependencies are detected: ${_all_un_deps}")
      endif()
    else()
      include (GetPrerequisites)
      foreach (_item ${_libs_${_conf}} ${_additional_libs_${_conf}} ${_execs_${_conf}} ${_additional_execs_${_conf}})
        get_prerequisites (${_item} _deps  1 1 "" "${_additional_dirs};${_third_party_dirs};${_mingw_path}")
        list (APPEND _all_rt_deps_${_conf} ${_deps})
      endforeach()
    endif()

    # Install detected dependencies
    list (REMOVE_DUPLICATES _all_rt_deps_${_conf})
    list (REMOVE_ITEM _all_rt_deps_${_conf} ${_libs_${_conf}})
    set (_symlinks_${_conf})
    set (_real_paths_${_conf})
    set (_exclude_list)
    foreach(_it ${_all_rt_deps_${_conf}})
      get_filename_component(_real ${_it} REALPATH)
      list (APPEND _real_paths_${_conf} ${_real})
      get_filename_component(_ext ${_it} EXT)
      if (_exclude_regexp)
        get_filename_component(_name ${_real} NAME)
        string(REGEX MATCH "${_exclude_regexp}" _exclude_match ${_name})
        if (_exclude_match)
          list (APPEND _exclude_list ${_real})
          continue()
        endif()
      endif()
      if(${_ext} MATCHES ".*\\.so+[\\.]+[0-9]")
        get_filename_component(_dir ${_it} DIRECTORY)
        get_filename_component(_fname_we ${_it} NAME_WE)
        string(FIND ${_ext} ".so" _fname_ext_prefix_pos)
        string(SUBSTRING ${_ext} 0 ${_fname_ext_prefix_pos} _fname_ext_prefix)
        set(_symlink ${_dir}/${_fname_we}${_fname_ext_prefix}.so)
        if (EXISTS ${_symlink})
          list (APPEND _symlinks_${_conf} ${_symlink})
        endif()
      endif()
    endforeach()
    set(_all_rt_deps_${_conf} ${_real_paths_${_conf}})
    if (_exclude_list)
      list (REMOVE_ITEM _all_rt_deps_${_conf} ${_exclude_list})
    endif()
    list (APPEND _all_rt_deps_${_conf} ${_symlinks_${_conf}})

    set ( _libs_to_install_${_conf} ${_all_rt_deps_${_conf}}
                                    ${_additional_libs_${_conf}}
                                    ${_additional_files})

    if (NOT exec_target_dir_${_conf})
      list (APPEND _libs_to_install_${_conf} ${_additional_execs_${_conf}})
    endif()

    set(_filtered_libs_${_conf})
    foreach(_dep ${_libs_to_install_${_conf}})
      if (_dep AND NOT _dep MATCHES "-NOTFOUND$")
        list(APPEND _filtered_libs_${_conf} ${_dep})
      elseif(_dep MATCHES "-NOTFOUND$")
        message(STATUS "Skipping missing dependency ${_dep}")
      endif()
    endforeach()
    set(_libs_to_install_${_conf} ${_filtered_libs_${_conf}})

    if (exec_target_dir_${_conf})
      set(_filtered_execs_${_conf})
      foreach(_dep ${_additional_execs_${_conf}})
        if (_dep AND NOT _dep MATCHES "-NOTFOUND$")
          list(APPEND _filtered_execs_${_conf} ${_dep})
        elseif(_dep MATCHES "-NOTFOUND$")
          message(STATUS "Skipping missing dependency ${_dep}")
        endif()
      endforeach()
      set(_additional_execs_${_conf} ${_filtered_execs_${_conf}})
    endif()

    file (INSTALL
          DESTINATION ${target_dir_${_conf}}
          USE_SOURCE_PERMISSIONS
          FOLLOW_SYMLINK_CHAIN
           FILES ${_libs_to_install_${_conf}})
    if (exec_target_dir_${_conf})
      file (INSTALL
            DESTINATION ${exec_target_dir_${_conf}}
            USE_SOURCE_PERMISSIONS
            FOLLOW_SYMLINK_CHAIN
            FILES ${_additional_execs_${_conf}})
    endif()
    ]])
endmacro()

# Retrieve list of dependencies and it's locations for target
# Parameters:
#  * target (IN) - name of target
#  * config (IN) - configuration (Release, Debug, ...)
#  * dependencies (OUT) - list of dependencies for target
#  * dependencies_directories (OUT) - list of dependencies for target
function (_get_target_dependencies target config dependencies dependencies_directories)
  cmake_policy(SET CMP0057 NEW)
  string (TOUPPER ${config} _uc_config)
  if (TARGET ${target})
    get_target_property(target_type ${target} TYPE)
    if (NOT ${target} IN_LIST ${dependencies} AND NOT target_type STREQUAL STATIC_LIBRARY)
      get_target_property (_target_deps ${target} INTERFACE_LINK_LIBRARIES)
      foreach (_dep ${_target_deps})
        if (TARGET ${_dep})
          # First try given configuration
          get_target_property (_loc ${_dep} IMPORTED_LOCATION_${_uc_config})
          if (NOT _loc)
            # Try other standard configurations
            set (_std_confs RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
            list (REMOVE_ITEM _std_confs ${_uc_config})
            foreach (_s_cfg ${_std_confs})
              get_target_property (_loc ${_dep} IMPORTED_LOCATION_${_s_cfg})
              if (_loc)
                break()
              endif()
            endforeach()
            if (NOT _loc)
              get_target_property (_loc ${_dep} IMPORTED_LOCATION)
            endif()
          endif()
          get_target_property(_dep_target_type ${_dep} TYPE)
          get_filename_component (_dep_target_file_dir ${_loc} DIRECTORY)
          string (REGEX MATCH ${_system_paths_regexp} _is_system "${_dep_target_file_dir}")
          if (NOT _dep_target_type STREQUAL STATIC_LIBRARY AND
              NOT ${_dep} IN_LIST ${dependencies} AND NOT _is_system)
            _get_target_dependencies (${_dep} ${config} ${dependencies} ${dependencies_directories})
            get_target_property (_is_imported ${_dep} IMPORTED)
            if (_is_imported)
              list (APPEND ${dependencies} ${_dep})
            endif()
          endif()
        else()
          set (_loc ${_dep})
        endif()
        if (_loc AND (NOT ${_loc} IN_LIST ${dependencies_directories}))
            list (APPEND ${dependencies_directories} ${_loc})
        endif()
      endforeach()
    endif()
    set (${dependencies} ${${dependencies}} PARENT_SCOPE)
    set (${dependencies_directories} ${${dependencies_directories}} PARENT_SCOPE)
  endif()
endfunction()

# Macro to set library dir variables for different configurations of a given target.
# Parameters:
#  * target - name of CMake target
#  * implib_var_template - template of a variable for import library to set.
#  * dll_var_template - template of a variable for dynamic loaded library to set.
# Note: Under Linux OS values of both variables are equal,
#       under Windows OS implib_var points to a directory, containing import library files.
#       These files contain information the linker needs to resolve external references.
#       Variable dll_var points to a directory, containing dynamic load libraries.
# Example:
# a Target 'my_target' has:
# Under Linux OS:
#    * IMPORTED_LOCATION_RELEASE=${_IMPORT_PREFIX}/lib/mylib.so
#    * IMPORTED_LOCATION_DEBUG=${_IMPORT_PREFIX}/libd/mylib.so
# Under Windows OS:
#    * IMPORTED_LOCATION_RELEASE=${_IMPORT_PREFIX}/bin/mylib.dll
#    * IMPORTED_LOCATION_DEBUG=${_IMPORT_PREFIX}/bind/mylib.dll
#    * IMPORTED_IMPLIB_RELEASE=${_IMPORT_PREFIX}/lib/mylib.lib
#    * IMPORTED_IMPLIB_DEBUG=${_IMPORT_PREFIX}/libd/mylib.lib
# properties.
# A call 'simake_set_library_dir_vars(my_target MY_LIBRARY_LIB_DIRS MY_LIBRARY_DLL_DIRS) in the Release configuration
# sets the following CMake variables:
# Under Linux OS:
#    * MY_LIBRARY_LIB_DIRS_RELEASE=${_IMPORT_PREFIX}/lib
#    * MY_LIBRARY_DLL_DIRS_RELEASE=${_IMPORT_PREFIX}/lib
# Under Windows OS:
#    * MY_LIBRARY_LIB_DIRS_RELEASE=${_IMPORT_PREFIX}/lib
#    * MY_LIBRARY_DLL_DIRS_RELEASE=${_IMPORT_PREFIX}/bin
macro (simake_set_library_dir_vars target implib_var_template dll_var_template)
  if (TARGET ${target})
    get_target_property (_target_confs ${target} IMPORTED_CONFIGURATIONS)
    foreach (_cfg ${_target_confs} "")
      set(_suffix "_")
      if (_cfg MATCHES "^$")
        set(_suffix "")
      endif()
      unset(_library_path)
      get_target_property (_library_path ${target} IMPORTED_LOCATION${_suffix}${_cfg})
      if (_library_path)
        get_filename_component (_item ${_library_path} DIRECTORY)
        set (${dll_var_template}${_suffix}${_cfg} ${_item})
        if (UNIX OR EMSCRIPTEN)
          set (${implib_var_template}${_suffix}${_cfg} ${_item})
        endif()
      endif()
      if (WIN32)
        unset(_library_path)
        get_target_property (_library_path ${target} IMPORTED_IMPLIB${_suffix}${_cfg})
        if (_library_path)
          get_filename_component (_item ${_library_path} DIRECTORY)
          set (${implib_var_template}${_suffix}${_cfg} ${_item})
        endif()
      endif()
    endforeach()
  endif()
endmacro()

# Print information about libraries
# Parameters:
#  * library_name - name of CMake package
#  * implib_var_template - template of a variable for import libraries.
#  * dll_var_template - template of a variable for dynamic loaded libraries.
macro (simake_print_libs_info library_name implib_var_template dll_var_template)
  simake_get_standard_configurations(_configurations)
  foreach (_conf ${_configurations})
    string (TOUPPER ${_conf} _uc_conf)
    set (_suff "_")
    set (_print_conf "(${_conf})")
    if (_conf MATCHES "^$")
      set (_suff "")
      set (_print_conf "")
    endif()
    set (_libdir ${${implib_var_template}${_suff}${_uc_conf}})
    if (_libdir)
      message (STATUS "  ${library_name} library ${_print_conf} dir(s): ${_libdir}")
    endif()
    if (WIN32)
      set (_libdir ${${dll_var_template}${_suff}${_uc_conf}})
      if (_libdir)
        message (STATUS "  ${library_name} dll library ${_print_conf} dir(s): ${_libdir}")
      endif()
    endif()
  endforeach()
endmacro()

# Collect paths to dynamic load libaries of third-party prerequsites, used within a project
# Parameters:
#   * list_template - name of a list to collect paths
#   * variable_template - name of a variable, used to retrieve a path
macro (simake_collect_dll_paths list_template variable_template)
  simake_get_project_configurations (_configurations UPPERCASE)
  foreach (_conf ${_configurations})
    set (_path_added OFF)
    if (${variable_template}_${_conf})
      list (APPEND ${list_template}_${_conf} ${${variable_template}_${_conf}})
      set (_path_added ON)
    else()
      simake_get_standard_configurations (_std_configurations UPPERCASE)
      foreach (_std_conf ${_std_configurations})
        if (${variable_template}_${_std_conf})
          list (APPEND ${list_template}_${_conf} ${${variable_template}_${_std_conf}})
          set (_path_added ON)
          break()
        endif()
      endforeach()
    endif()
    if (NOT _path_added AND ${variable_template})
      list (APPEND ${list_template}_${_conf} ${${variable_template}})
    endif()
  endforeach()
endmacro()

# Return configurations, generated by a project
# Parameters:
#   * configurations_list (output parameter) - list of available project configurations
#   * UPPERCASE (optional parameter) - values a list is in upper case.
macro (simake_get_project_configurations configurations_list)
  cmake_parse_arguments (ARG "UPPERCASE" "" "" ${ARGN})
  set (${configurations_list} ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES} "")
  list (REMOVE_DUPLICATES ${configurations_list})
  if (ARG_UPPERCASE)
    list (TRANSFORM  ${configurations_list} TOUPPER)
  endif()
endmacro()

# Return standard CMake configurations: Release, Debug, RelWithDebInfo, MinSizeRel and NoConfig
# Parameters:
#   * configurations_list (output parameter) - list of standard CMake configurations
#   * UPPERCASE (optional parameter) - values a list is in upper case.
macro (simake_get_standard_configurations configurations_list)
  set (${configurations_list} "Release;Debug;RelWithDebInfo;MinSizeRel;NoConfig")
  cmake_parse_arguments (ARG "UPPERCASE" "" "" ${ARGN})
  if (ARG_UPPERCASE)
    list (TRANSFORM ${configurations_list} TOUPPER)
  endif()
endmacro()

# Print information about include directories
# Parameters:
#  * library_name - name of CMake package
#  * target_name - name of CMake target
macro (simake_print_includes_info library_name target_name)
  if (TARGET ${target} ${target_name})
    get_target_property (_includes_dirs ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
    if (_includes_dirs)
      message (STATUS "  ${library_name} include dir(s): ${_includes_dirs}")
    endif()
  endif()
endmacro()


# Macro to install LightWeight Viewer targets
# Input parameters:
#   - EXECUTABLES:      list of executables to install
#   - LIBRARIES:        list of libraries to install
#   - HEADERS_SUBDIR:   sub-directory to install header files (files are installed into
#                       ${OCCTViewer_INSTALL_INCLUDE}/${HEADERS_SUBDIR} directory)
macro (OCCTViewer_install_targets)
  # Parse macro's args
  cmake_parse_arguments (OCCTViewer_INSTALL "" "" "EXECUTABLES;LIBRARIES;HEADERS;HEADERS_SUBDIR" ${ARGN})
  # Install executables
  if (OCCTViewer_INSTALL_EXECUTABLES)
    install (TARGETS ${OCCTViewer_INSTALL_EXECUTABLES}
              CONFIGURATIONS "Release"
              DESTINATION "${OCCTViewer_INSTALL_BIN_RELEASE}")
    install (TARGETS "${OCCTViewer_INSTALL_EXECUTABLES}"
              CONFIGURATIONS "Debug"
              DESTINATION "${OCCTViewer_INSTALL_BIN_DEBUG}")
  endif()

  # Install libraries
  if (OCCTViewer_INSTALL_LIBRARIES)
    install (TARGETS ${OCCTViewer_INSTALL_LIBRARIES}
              EXPORT ${OCCTVieweriewer_PACKAGE}
              CONFIGURATIONS "Release"
              RUNTIME DESTINATION "${OCCTViewer_INSTALL_DLL_RELEASE}"
              LIBRARY DESTINATION "${OCCTViewer_INSTALL_LIB_RELEASE}"
              ARCHIVE DESTINATION "${OCCTViewer_INSTALL_LIB_RELEASE}")
    install (TARGETS "${OCCTViewer_INSTALL_LIBRARIES}"
              CONFIGURATIONS "Debug"
              RUNTIME DESTINATION "${OCCTViewer_INSTALL_DLL_DEBUG}"
              LIBRARY DESTINATION "${OCCTViewer_INSTALL_LIB_DEBUG}"
              ARCHIVE DESTINATION "${OCCTViewer_INSTALL_LIB_DEBUG}")
    if (OCCTViewer_INSTALL_HEADERS)
      foreach (target ${OCCTViewer_INSTALL_LIBRARIES})
        target_include_directories (${target} PUBLIC $<INSTALL_INTERFACE:${OCCTViewer_INSTALL_INCLUDE}>)
      endforeach()
    endif()
  endif()

  # Install headers
  if (OCCTViewer_INSTALL_HEADERS)
    install (FILES ${OCCTViewer_INSTALL_HEADERS}
              DESTINATION "${OCCTViewer_INSTALL_INCLUDE}/${OCCTViewer_INSTALL_HEADERS_SUBDIR}")
  endif()
endmacro()
