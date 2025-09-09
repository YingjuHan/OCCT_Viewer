project (RWShape)

set (SRCFILES
  GltfParser.cpp
  ShapeData.cpp
)
set (INCFILES
  GltfParser.h
  ShapeData.h
  ShapeDataExplorer.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies to link
target_link_libraries (${PROJECT_NAME} PUBLIC RWBase)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC
                       TKernel TKMath TKService TKBRep TKRWMesh TKTopAlgo TKXCAF TKG3d)

# install target and header
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})