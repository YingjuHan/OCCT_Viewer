project (RWShapeConv)

set (SRCFILES
  ShapeLoader.cpp
)

set (INCFILES
  ShapeLoader.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies to link
target_link_libraries (${PROJECT_NAME} PUBLIC RWShape RWBase)

# OCCT libraries to link
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKernel TKMath TKService TKBRep TKRWMesh TKGeomBase TKSTL TKV3d TKXCAF TKG3d TKTopAlgo)

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})