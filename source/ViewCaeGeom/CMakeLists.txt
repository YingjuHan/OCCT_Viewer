project (ViewCaeGeom)

set (SRCFILES
  MeshBvhData.cpp
  MeshGeometry.cpp
  MeshGeometryBlock.cpp
  MeshGeometryClip.cpp
  MeshGeometryDeformed.cpp
  MeshGeometryGen.cpp
  MeshGeometryIso.cpp
  MeshGeometrySlice.cpp
  MeshGeometryTools.cpp
)
set (INCFILES
  MeshBvhData.h
  MeshGeometry.h
  MeshGeometryBlock.h
  MeshGeometryClip.h
  MeshGeometryDeformed.h
  MeshGeometryGen.h
  MeshGeometryIso.h
  MeshGeometrySlice.h
  MeshGeometryTools.h
  MeshHBvhBoxSet.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC RWBase RWCae)

# OCCT dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC TKernel TKMath TKService)

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})