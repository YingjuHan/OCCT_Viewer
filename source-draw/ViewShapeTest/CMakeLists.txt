cmake_minimum_required (VERSION 3.7)

project (ViewShapeTest)

set (SRCFILES
  PrsShapeCommands.cpp
  ScvAspects.cpp
  ScvDimension.cpp
  ScvDisplay.cpp
  ScvSelect.cpp
  ScvSelmode.cpp
  ScvSelState.cpp
)
set (INCFILES
  PrsShapeCommands.h
  ScvAspects.h
  ScvDimension.h
  ScvDisplay.h
  ScvSelect.h
  ScvSelmode.h
  ScvSelState.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC DrawBase RWShapeTest Viewer ViewShape ViewBase ViewBaseTest)

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