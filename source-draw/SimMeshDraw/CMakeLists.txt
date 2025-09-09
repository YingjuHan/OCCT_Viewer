project (SimMeshDraw)

set (SRCFILES
  SimMeshDraw.cpp
  SimMeshDraw_DataSource.cpp
  SimMeshDraw_DrawableMesh.cpp
)
set (INCFILES
  SimMeshDraw_DataSource.h
  SimMeshDraw_DrawableMesh.h
  SimMeshDraw.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# add library
add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC SimMesh)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC TKDraw TKViewerTest TKMeshVS TKernel)

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})

