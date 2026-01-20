# Config file for the OCCTViewer package
# ===============================


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was OCCTViewerConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

if (NOT TARGET RWBase)
  include (${CMAKE_CURRENT_LIST_DIR}/OCCTViewerTargets.cmake)
endif()

# Libraries
# =========
set (_OCCTViewer_KNOWN_LIBRARIES
  RWBase
  ViewBase
  RWShape
  RWShapeConv
  ViewShape
  ViewDim
  RWCae
  RWMed
  RWVtk
  RWMeshConv
  RWMeshExchanger
  ViewCae
  ViewCaeGeom
  Viewer
  FileExchange
  DrawBase
  RWCaeTest
  RWShapeTest
  ViewBaseTest
  ViewCaeTest
  ViewShapeTest
  ViewDraw
)

set(OCCTViewer_LIBRARIES "")
foreach (_lib IN LISTS _OCCTViewer_KNOWN_LIBRARIES)
  if (TARGET ${_lib})
    list (APPEND OCCTViewer_LIBRARIES ${_lib})
  endif()
endforeach()
