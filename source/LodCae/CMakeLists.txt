project (LodCae)

set (SRCFILES
  Simplifier.cpp
)
set (INCFILES
  Simplifier.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PRIVATE RWCae SimMesh)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PRIVATE
                       TKernel TKMath TKG3d)

# install target
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
