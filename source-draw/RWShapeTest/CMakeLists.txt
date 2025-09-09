cmake_minimum_required (VERSION 3.7)

project (RWShapeTest)

set (SRCFILES
  DrawableShapeData.cpp
  RWShapeCommands.cpp
  ScvExplode.cpp
  ScvLoad.cpp
  ScvReadShape.cpp
)
set (INCFILES
  DrawableShapeData.h
  RWShapeCommands.h
  ScvExplode.h
  ScvLoad.h
  ScvReadShape.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC DrawBase ViewBaseTest Viewer ViewShape ViewBase)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC TKDraw TKernel TKViewerTest)

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()

# install target and headers
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})

