project (RWVtk)

set (SRCFILES
  VtkBaseReader.cpp
  VtkBaseXMLReader.cpp
  VtkDataArray.cpp
  VtkMultiBlockDataReader.cpp
  VtkPolyDataReader.cpp
  VtkSeriesReader.cpp
  VtkUnstructuredDataReader.cpp
  VtkUnstructuredGridReader.cpp
)
set (INCFILES
  VtkBaseReader.h
  VtkBaseXMLReader.h
  VtkDataArray.h
  VtkMultiBlockDataReader.h
  VtkPolyDataReader.h
  VtkSeriesReader.h
  VtkUnstructuredDataReader.h
  VtkUnstructuredGridReader.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC RWCae)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC
                       TKernel TKMath TKService TKBRep TKGeomBase TKV3d TKXCAF TKG3d TKRWMesh TKTopAlgo TKCDF)

# install target
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})