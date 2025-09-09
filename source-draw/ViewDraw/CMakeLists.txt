cmake_minimum_required (VERSION 3.7)

project (ViewDraw)

set (SRCFILES
  SCDraw.cpp
  ScvRead.cpp
)

set (INCFILES
  SCDraw.h
  ScvRead.h
)

set (RESFILES VIEWDRAW)

source_group ("Source Files"   FILES ${SRCFILES})
source_group ("Header Files"   FILES ${INCFILES})
source_group ("Resource Files" FILES ${RESFILES})

add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES} ${RESFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC Viewer ViewCae ViewShape ViewBase RWBase RWCaeTest
                       RWShapeTest ViewCaeTest ViewShapeTest LodCaeTest SimMeshDraw)

# OCCT libraries
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKDraw TKViewerTest TKV3d TKService TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase
                       TKG3d TKG2d TKMath TKernel)

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PUBLIC dl pthread)
endif()

# install target and resources
OCCTViewer_install_targets(LIBRARIES ${PROJECT_NAME}
                    HEADERS ${INCFILES}
                    HEADERS_SUBDIR ${PROJECT_NAME})

# for debug purposes
get_target_property (aTKernelRel "TKernel" IMPORTED_LOCATION_RELEASE)
get_target_property (aTKernelDbg "TKernel" IMPORTED_LOCATION_DEBUG)
get_filename_component (OpenCASCADE_BINARY_DIR_RELEASE ${aTKernelRel} DIRECTORY)
get_filename_component (OpenCASCADE_BINARY_DIR_DEBUG   ${aTKernelDbg} DIRECTORY)
if (NOT EXISTS "${OpenCASCADE_BINARY_DIR_DEBUG}" AND EXISTS "${OpenCASCADE_BINARY_DIR_RELEASE}")
  set (OpenCASCADE_BINARY_DIR_DEBUG "${OpenCASCADE_BINARY_DIR_RELEASE}")
elseif (NOT EXISTS "${OpenCASCADE_BINARY_DIR_RELEASE}" AND EXISTS "${OpenCASCADE_BINARY_DIR_DEBUG}")
  set (OpenCASCADE_BINARY_DIR_RELEASE "${OpenCASCADE_BINARY_DIR_DEBUG}")
endif()

# batch script files running DRAWEXE from make folder
# if (WIN32)
#   configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/draw-release.bat.in" "${CMAKE_BINARY_DIR}/draw-release.bat" @ONLY)
#   configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/draw-debug.bat.in"   "${CMAKE_BINARY_DIR}/draw-debug.bat"   @ONLY)

#   configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/draw-release-install.bat.in" "${CMAKE_BINARY_DIR}/draw-release-install.bat.in" @ONLY)
#   configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/draw-debug-install.bat.in"   "${CMAKE_BINARY_DIR}/draw-debug-install.bat.in"   @ONLY)
#   install (FILES       "${CMAKE_BINARY_DIR}/draw-release-install.bat.in" CONFIGURATIONS "Release"
#            DESTINATION "${CMAKE_INSTALL_PREFIX}"
#            RENAME      "draw-release.bat")
#   install (FILES       "${CMAKE_BINARY_DIR}/draw-debug-install.bat.in" CONFIGURATIONS "Debug"
#            DESTINATION "${CMAKE_INSTALL_PREFIX}"
#            RENAME      "draw-debug.bat")
# else()
#   configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/draw.sh.in" "${CMAKE_BINARY_DIR}/draw.sh" @ONLY
#                   FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
# endif()

install (FILES       "${CMAKE_CURRENT_SOURCE_DIR}/VIEWDRAW"
         DESTINATION "${CMAKE_INSTALL_PREFIX}/source")
# install (DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/../../tests"
#          DESTINATION "${CMAKE_INSTALL_PREFIX}")
install (DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/fonts"
         DESTINATION "${CMAKE_INSTALL_PREFIX}/resources")
install (DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/models"
         DESTINATION "${CMAKE_INSTALL_PREFIX}/resources")

if (MSVC)
  # Visual Studio debugger environment
  set_target_properties (${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "\
PATH=%PATH%;$<IF:$<CONFIG:Debug>,${OpenCASCADE_BINARY_DIR_DEBUG},${OpenCASCADE_BINARY_DIR_RELEASE}>;${3RDPARTY_DLL_DIRS}\n\
CSF_VIEWDRAWDefaults=${CMAKE_CURRENT_SOURCE_DIR}\n\
CSF_TestScriptsPath=${CMAKE_CURRENT_SOURCE_DIR}/../../tests\n\
CSF_TestDataPath=${CMAKE_CURRENT_SOURCE_DIR}/../../resources"
    VS_DEBUGGER_COMMAND           "$<IF:$<CONFIG:Debug>,${OpenCASCADE_BINARY_DIR_DEBUG},${OpenCASCADE_BINARY_DIR_RELEASE}>/DRAWEXE.exe"
    VS_DEBUGGER_COMMAND_ARGUMENTS "-i -c dputs {pload -VIEWDRAW};pload -VIEWDRAW"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
endif()

