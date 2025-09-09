project (ViewShape)

set (SRCFILES
  InteractiveShape.cpp
  InteractiveShapeDrawer.cpp
  InteractiveShapeOwner.cpp
  SensitiveCompound.cpp
  SensitivePolyEdge.cpp
  SensitivePolyEdgeOnTri.cpp
  SensitivePolyFace.cpp
  SensitiveVertex.cpp
)

set (INCFILES
  InteractiveShape.h
  InteractiveShapeDrawer.h
  InteractiveShapeOwner.h
  InteractiveShapePrsMode.h
  InteractiveShapeSelMode.h
  SensitiveCompound.h
  SensitivePolyEdge.h
  SensitivePolyEdgeOnTri.h
  SensitivePolyFace.h
  SensitiveShape.h
  SensitiveVertex.h
  ShapeSelElems.h
  ShapeSelElemType.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC ViewBase RWBase RWShape)

# OCCT libraries to link with
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKernel TKMath TKService TKBRep TKGeomBase TKV3d TKXCAF TKG3d TKTopAlgo TKRWMesh)

# install target
OCCTViewer_install_targets(LIBRARIES ${PROJECT_NAME}
                    HEADERS ${INCFILES}
                    HEADERS_SUBDIR ${PROJECT_NAME})