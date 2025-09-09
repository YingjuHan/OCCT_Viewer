cmake_minimum_required (VERSION 3.7)

project (RWCaeTest)

set (SRCFILES
  DrawableMeshData.cpp
  MeshIOCommand.cpp
  RWCaeCommands.cpp
  ScvCompareMeshes.cpp
  ScvExportMesh.cpp
  ScvFarfieldJsonMesh.cpp
  ScvMeshInfo.cpp
  ScvMeshPropDependency.cpp
  ScvNearfieldPhaseMesh.cpp
  ScvReadMapping.cpp
  ScvReadMesh.cpp
  ScvReadMeshProperties.cpp
  ScvSortMesh.cpp
  ScvTestMesh.cpp
  ScvTestMeshProp.cpp
)
set (INCFILES
  DrawableMeshData.h
  MeshIOCommand.h
  RWCaeCommands.h
  ScvCompareMeshes.h
  ScvExportMesh.h
  ScvFarfieldJsonMesh.h
  ScvMeshInfo.h
  ScvMeshPropDependency.h
  ScvNearfieldPhaseMesh.h
  ScvReadMapping.h
  ScvReadMesh.h
  ScvReadMeshProperties.h
  ScvSortMesh.h
  ScvTestMesh.h
  ScvTestMeshProp.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# add library
add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC DrawBase ViewBaseTest ViewCaeTest Viewer ViewCae ViewBase)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC TKDraw TKernel TKViewerTest)

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})