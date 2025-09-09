cmake_minimum_required (VERSION 3.7)

project (ViewBaseTest)

set (SRCFILES
  PrsBaseCommands.cpp
  ScvClipping.cpp
  ScvClone.cpp
  ScvDrawText.cpp
  ScvFont.cpp
  ScvInit.cpp
  ScvScaleRuler.cpp
  ScvUnit.cpp
  ScvViewCube.cpp
  ViewerCommand.cpp
  ViewerDrawable.cpp
)
set (INCFILES
  PrsBaseCommands.h
  ScvClipping.h
  ScvClone.h
  ScvDrawText.h
  ScvFont.h
  ScvInit.h
  ScvScaleRuler.h
  ScvUnit.h
  ScvViewCube.h
  ViewerCommand.h
  ViewerDrawable.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# add library
add_library (${PROJECT_NAME} SHARED ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Draw")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC DrawBase Viewer ViewBase)

# OCCT libraries
target_link_libraries (${PROJECT_NAME} PUBLIC TKDraw TKernel TKViewerTest)

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()

# install target and header files
OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})