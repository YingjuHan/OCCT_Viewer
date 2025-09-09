cmake_minimum_required (VERSION 3.7)

project (LodCaeTest)

set (SRCFILES
  LodCaeCommands.cpp
  ScvMeshSimplify.cpp
)
set (INCFILES
  LodCaeCommands.h
  ScvMeshSimplify.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# add library
add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC LodCae DrawBase RWCaeTest)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC TKDraw TKernel)

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()

# install target and header files
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})
