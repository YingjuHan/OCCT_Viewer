project (ViewDim)

set (SRCFILES
  InteractiveAngleDimension.cpp
  InteractiveDimensionOwner.cpp
  InteractiveLengthDimension.cpp
  InteractiveRadiusDimension.cpp
)

set (INCFILES
  InteractiveAngleDimension.h
  InteractiveDimensionOwner.h
  InteractiveLengthDimension.h
  InteractiveRadiusDimension.h
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
                    HEADERS ${INCFILES})