project (ViewCaeGeom)

set (SRCFILES
  MeshCutViewManager.cpp
  MeshCutIntegration.cpp
  MeshCutRecovery.cpp
  MeshGeometryQuadric.cpp
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
  MeshCutViewManager.h
  MeshCutIntegration.h
  MeshCutRecovery.h
  MeshGeometryQuadric.h
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
target_link_libraries (${PROJECT_NAME} PUBLIC TKernel TKMath TKService TKPrim TKBO TKMesh)
if (OCCTV_USE_VTK)
  target_sources (${PROJECT_NAME} PRIVATE MeshVtkCell.cpp MeshVtkAnalytic.cpp MeshVtkCutSource.cpp MeshGeometryVtkCut.cpp MeshGeometryVtkIso.cpp)
  target_link_libraries (${PROJECT_NAME} PUBLIC VTK::CommonCore VTK::CommonDataModel)
  target_compile_definitions (${PROJECT_NAME} PUBLIC OCCTV_USE_VTK)
endif()

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
