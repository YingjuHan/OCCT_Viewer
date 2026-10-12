project (RWVtk)
find_package (ZLIB REQUIRED)

set (SRCFILES
  VtkLegacyReader.cpp
  VtkValidationReader.cpp
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
  VtkLegacyReader.h
  VtkValidationReader.h
  VtkResourceLimits.h
  VtkImportReport.h
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
target_link_libraries (${PROJECT_NAME} PUBLIC RWCae PRIVATE ZLIB::ZLIB)
if (OCCTV_USE_VTK)
  target_sources (${PROJECT_NAME} PRIVATE VtkNativeReader.cpp)
  target_link_libraries (${PROJECT_NAME} PUBLIC VTK::CommonCore VTK::CommonDataModel VTK::IOXML VTK::IOLegacy)
  target_compile_definitions (${PROJECT_NAME} PUBLIC OCCTV_USE_VTK)
  list (APPEND INCFILES VtkNativeReader.h)
endif()

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC
                       TKernel TKMath TKService TKBRep TKGeomBase TKV3d TKXCAF TKG3d TKRWMesh TKTopAlgo TKCDF)

# install target
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
