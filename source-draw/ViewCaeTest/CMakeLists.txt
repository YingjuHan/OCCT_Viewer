cmake_minimum_required (VERSION 3.7)

project (ViewCaeTest)

set (SRCFILES
  CutViewCommands.cpp
  CutBenchmarkCommands.cpp
  MeshPrsCommand.cpp
  PrsCaeCommands.cpp
  ScvColorScale.cpp
  ScvMesh2DGrid.cpp
  ScvMeshActiveProp.cpp
  ScvMeshAnimation.cpp
  ScvMeshAnnotation.cpp
  ScvMeshAspects.cpp
  ScvMeshBlock.cpp
  ScvMeshClipPlane.cpp
  ScvMeshDefProp.cpp
  ScvMeshIso.cpp
  ScvMeshPrs.cpp
  ScvMeshRemoveProps.cpp
  ScvMeshSelMode.cpp
  ScvMeshSlice.cpp
  ScvMeshStreamlines.cpp
  ScvMeshTestGroup.cpp
  ScvMeshVecProp.cpp
  ScvSphericalAxes.cpp
)
set (INCFILES
  CutViewCommands.h
  MeshPrsCommand.h
  PrsCaeCommands.h
  ScvColorScale.h
  ScvMesh2DGrid.h
  ScvMeshActiveProp.h
  ScvMeshAnimation.h
  ScvMeshAnnotation.h
  ScvMeshAspects.h
  ScvMeshBlock.h
  ScvMeshClipPlane.h
  ScvMeshDefProp.h
  ScvMeshIso.h
  ScvMeshPrs.h
  ScvMeshRemoveProps.h
  ScvMeshResetCounters.h
  ScvMeshSelMode.h
  ScvMeshSlice.h
  ScvMeshStreamlines.h
  ScvMeshTestGroup.h
  ScvMeshVecProp.h
  ScvSphericalAxes.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC DrawBase Viewer ViewCae ViewBase ViewBaseTest RWVtk)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC TKDraw TKernel TKViewerTest)
if (WIN32)
  target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 psapi)
else()
  find_package (OpenGL REQUIRED)
  target_link_libraries (${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
