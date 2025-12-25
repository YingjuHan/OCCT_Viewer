project (ViewBase)

set (SRCFILES
  BaseDefaultDrawer.cpp
  BaseSelection.cpp
  BaseViewer.cpp
  IInteractiveObject.cpp
  InteractiveClipping.cpp
  InteractiveClippingOwner.cpp
  InteractiveOwner.cpp
  InteractiveScaleRuler.cpp
  InteractiveScaleRulerOwner.cpp
  InteractiveTextLabel.cpp
  InteractiveTextLabelOwner.cpp
  InteractiveViewCube.cpp
  InteractiveViewCubeOwner.cpp
  OffscreenViewer.cpp
  ScalablePixMap.cpp
  ScalableTexture2D.cpp
  StatsActivator.cpp
  UnitManager.cpp
)

set (INCFILES
  BaseDefaultDrawer.h
  BaseSelection.h
  BaseViewer.h
  IInteractiveObject.h
  InteractiveClipping.h
  InteractiveClippingOwner.h
  InteractiveOwner.h
  InteractiveScaleRuler.h
  InteractiveScaleRulerOwner.h
  InteractiveTextLabel.h
  InteractiveTextLabelOwner.h
  InteractiveViewCube.h
  InteractiveViewCubeOwner.h
  OffscreenViewer.h
  PrsTools.h
  ScalablePixMap.h
  ScalableTexture2D.h
  StatsActivator.h
  UnitManager.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# OCCT libraries to link
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKernel TKMath TKService TKBRep TKGeomBase TKV3d TKXCAF TKG3d TKTopAlgo)

# MED libraries to link
if (OCCTV_USE_MEDFILE)
  target_link_libraries (${PROJECT_NAME} PRIVATE ${MEDFILE_C_LIBRARIES})
endif()

# install target and header files
OCCTViewer_install_targets(LIBRARIES ${PROJECT_NAME}
                    HEADERS ${INCFILES}
                    HEADERS_SUBDIR ${PROJECT_NAME})
