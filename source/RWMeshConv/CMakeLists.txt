project (RWMeshConv)

set (SRCFILES
  GmshLoader.cpp
  GmshLoaderContext.cpp
  MedConverter.cpp
  MeshLoader.cpp
)

set (INCFILES
  GmshLoader.h
  GmshLoaderContext.h
  MedConverter.h
  MeshLoader.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC RWBase RWCae RWVtk RWMed)

# OCCT libraries to link
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKernel TKMath TKService TKBRep TKRWMesh TKGeomBase TKV3d TKXCAF TKG3d TKTopAlgo)

# install target and header
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})