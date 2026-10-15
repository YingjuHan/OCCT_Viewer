project (RWCae)

set (SRCFILES
  MeshAdaptor.cpp
  MeshBinReader.cpp
  MeshBinWriter.cpp
  MeshData.cpp
  MeshDataBlock.cpp
  MeshDracoCompressionHelper.cpp
  MeshFieldMapping.cpp
  MeshGroupProperty.cpp
  MeshInfo.cpp
  MeshNodeVecProperty.cpp
  MeshNVecProperty.cpp
  MeshPropertyBase.cpp
  MeshScalarProperty.cpp
  MeshScalarPropertyBase.cpp
  MeshTools.cpp
  MeshToolsSortingImpl.cpp
)
set (INCFILES
  MeshCellTopology.h
  MeshAdaptor.h
  MeshAdaptorsIterator.h
  MeshBinIOBase.h
  MeshBinReader.h
  MeshBinWriter.h
  MeshCompressionHelper.h
  MeshData.h
  MeshDataBlock.h
  MeshDataRange.h
  MeshDataVector.h
  MeshDracoCompressionHelper.h
  MeshElement.h
  MeshElements.h
  MeshFieldMapping.h
  MeshFieldMappingInfo.h
  MeshGroupProperty.h
  MeshImportExport.h
  MeshInfo.h
  MeshNodeVecProperty.h
  MeshNodeVecPropertyData.h
  MeshNVecProperty.h
  MeshPropertyBase.h
  MeshPropertyData.h
  MeshResultType.h
  MeshScalarProperty.h
  MeshScalarPropertyBase.h
  MeshScalarPropertyData.h
  MeshTools.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PRIVATE RWBase)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PRIVATE
                       TKernel TKMath TKService TKBRep TKRWMesh TKTopAlgo TKXCAF TKG3d)

# draco
if (OCCTV_USE_DRACO)
  target_link_libraries (${PROJECT_NAME} PRIVATE draco::draco)
endif()

# install target
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                            HEADERS ${INCFILES}
                            HEADERS_SUBDIR ${PROJECT_NAME})