include (common_macros)

#  HDF5
set (HDF5_ROOT_DIR "${CONAN_HDF5_ROOT}" CACHE PATH "hdf5 library installation directory")
# Looking for hdf5 additional search directories
simake_additional_search_dirs (HDF5 "hdf5-")
# Find hdf5 library
find_package (HDF5 QUIET PATH_SUFFIXES cmake/hdf5)
if (NOT HDF5_FOUND)
  find_package (HDF5 REQUIRED)
endif()

if (TARGET hdf5-shared)
 set (_hdf5_target hdf5-shared)
elseif (TARGET hdf5::hdf5)
  set (_hdf5_target hdf5::hdf5)
endif()
if (_hdf5_target)
  simake_set_library_dir_vars(${_hdf5_target} HDF5_LIBRARY_DIRS HDF5_DLL_DIRS)
  simake_collect_dll_paths(3RDPARTY_DLL_DIRS HDF5_DLL_DIRS)
endif()

# MedFile
set (MEDFile_ROOT_DIR "MEDFile_ROOT_DIR-NOTFOUND" CACHE PATH "MEDFile library installation directory")
# Look for additional search directories
simake_additional_search_dirs (MEDFile "med-;MedFile-")

# Find MEDFile library
set(supported_versions 4.1.0 4.1.1)
foreach(ver IN LISTS supported_versions)
  list(APPEND search_suffixes med-fichier-${ver} med-${ver} share/cmake/med-fichier-${ver})
endforeach()
find_package(MEDFile REQUIRED PATH_SUFFIXES ${search_suffixes})
if (MEDFile_FOUND)
  message (STATUS "MEDFile library has been found:")
  message (STATUS "  MEDFile include dir(s): ${MEDFILE_INCLUDE_DIRS}")

  if (TARGET medC)
    set_property (TARGET medC PROPERTY
                  INTERFACE_INCLUDE_DIRECTORIES "${HDF5_INCLUDE_DIRS};${MEDFILE_INCLUDE_DIRS}")
    simake_set_library_dir_vars (medC MEDFILE_LIBRARY_DIRS MEDFILE_DLL_DIRS)
    simake_print_libs_info("MEDFile" MEDFILE_LIBRARY_DIRS MEDFILE_DLL_DIRS)
    simake_collect_dll_paths(3RDPARTY_DLL_DIRS MEDFILE_DLL_DIRS)
  endif()
endif()