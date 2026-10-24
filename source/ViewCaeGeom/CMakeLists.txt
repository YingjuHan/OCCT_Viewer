project (ViewCaeGeom)

set (SRCFILES
  MeshCutViewManager.cpp
  MeshCutFrame.cpp
  MeshCutMaterial.cpp
    MeshCutMaterialSampling.cpp
  MeshCutSlicing.cpp
  MeshCutPhysics.cpp
  MeshCutAutomatic.cpp
  MeshCutCache.cpp
  MeshCutIntegration.cpp
  MeshCutRecovery.cpp
  MeshCutTensor.cpp
  MeshCutTrace.cpp
  MeshCutUnion.cpp
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
  MeshCutPlanePlacement.h
  MeshCutMaterial.h
  MeshCutFrame.h
  MeshCutSlicing.h
  MeshCutPhysics.h
  MeshCutAutomatic.h
  MeshCutCache.h
  MeshCutIntegration.h
  MeshCutRecovery.h
  MeshCutTensor.h
  MeshCutTrace.h
  MeshCutTraceOptions.h
  MeshCutUnion.h
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
target_sources (${PROJECT_NAME} PRIVATE MeshCellKernel.cpp MeshVtkCell.cpp MeshVtkAnalytic.cpp MeshVtkCutSource.cpp MeshGeometryVtkCut.cpp MeshGeometryVtkIso.cpp)
list (APPEND INCFILES MeshCellKernel.h MeshCellTables.h MeshVtkCell.h MeshVtkAnalytic.h MeshVtkCutSource.h MeshGeometryVtkCut.h MeshGeometryVtkIso.h)


# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
