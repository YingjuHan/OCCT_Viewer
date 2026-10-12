project (ViewCae)

set (SRCFILES
  CaeMeshAnimation.cpp
  CaeMeshArrowsTools.cpp
  CaeMeshStreamlinesBuilder.cpp
  CaeMeshTools.cpp
  ColorScaleTools.cpp
  InteractiveCaeCutView.cpp
  InteractiveCaeCutPlane.cpp
  InteractiveCaeMesh.cpp
  InteractiveCaeMeshDrawer.cpp
  InteractiveCaeMeshOwner.cpp
  InteractiveColorScale.cpp
  InteractiveColorScaleOwner.cpp
  InteractiveSphericalAxes.cpp
  MeshAdaptorSensitive.cpp
  MeshAnnotations.cpp
  MeshColorMapper.cpp
  MeshColorScaleMapper.cpp
  MeshPart.cpp
  MeshPartGeneralCells.cpp
  MeshPresentation.cpp
  MeshPresentationBase.cpp
  MeshPresentations.cpp
  MeshPresentationScalar.cpp
  MeshPresentationStreamlines.cpp
  MeshPresentationTools.cpp
  MeshPresentationVector.cpp
  MeshPropertyData.cpp
  MeshSensPart.cpp
  MeshSubEntity.cpp
)

set (INCFILES
  CaeMeshAnimation.h
  CaeMeshArrowsTools.h
  CaeMeshStreamlinesBuilder.h
  CaeMeshTools.h
  ColorScaleTools.h
  InteractiveCaeCutView.h
  InteractiveCaeCutPlane.h
  InteractiveCaeMesh.h
  InteractiveCaeMeshDrawer.h
  InteractiveCaeMeshOwner.h
  InteractiveCaeMeshPrsMode.h
  InteractiveCaeMeshSelMode.h
  InteractiveColorScale.h
  InteractiveColorScaleOwner.h
  InteractiveSphericalAxes.h
  MeshAdaptorSensitive.h
  MeshAnnotations.h
  MeshColorMapper.h
  MeshColorScaleMapper.h
  MeshPart.h
  MeshPartGeneralCells.h
  MeshPresentation.h
  MeshPresentationBase.h
  MeshPresentations.h
  MeshPresentationScalar.h
  MeshPresentationStreamlines.h
  MeshPresentationTools.h
  MeshPresentationVector.h
  MeshPropertyData.h
  MeshSensPart.h
  MeshSubEntity.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC ViewBase RWBase RWCae ViewCaeGeom RWShape)

# OCCT libraries to link
target_link_libraries (${PROJECT_NAME} PUBLIC
                       TKernel TKMath TKService TKBRep TKGeomBase TKV3d TKXCAF TKG3d TKTopAlgo)

# OCCT OpenGL libraries to link
if (EMSCRIPTEN)
  target_link_libraries (${PROJECT_NAME} PRIVATE TKOpenGles)
else()
  target_link_libraries (${PROJECT_NAME} PUBLIC TKOpenGl)
endif()

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
