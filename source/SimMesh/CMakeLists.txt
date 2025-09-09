project (SimMesh)

set (SRCFILES
  SimMesh_Attributes.cpp
  SimMesh.cpp
  SimMesh_Model.cpp
  SimMesh_Persistence.cpp
  SimMesh_Simplifier.cpp
  SimMesh_VertexDataList.cpp
)
set (INCFILES
  SimMesh_Attributes.h
  SimMesh_Color.h
  SimMesh_Edge.h
  SimMesh.h
  SimMesh_IdList.h
  SimMesh_Model.h
  SimMesh_Quadric.h
  SimMesh_Persistence.h
  SimMesh_Simplifier.h
  SimMesh_TargetType.h
  SimMesh_Triangle.h
  SimMesh_Types.h
  SimMesh_VertexData.h
  SimMesh_VertexDataList.h
  SimMesh_Vertex.h
  SimMesh_VertexPairContraction.h
  SimMesh_VertexPair.h
  SimMesh_VertexPairHeap.h
  SimMesh_VertexPairList.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PRIVATE
                       TKernel TKMath TKBRep TKG2d TKG3d)

# install target
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})