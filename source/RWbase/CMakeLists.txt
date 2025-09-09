project (RWBase)

set (SRCFILES
  EmulatedFileSystem.cpp
  FileSystemMap.cpp
  TaskManager.cpp
)

set (INCFILES
  EmulatedFileSystem.h
  FileSystemMap.h
  ConditionPromise.h
  IPromise.h
  JsonHelper.h
  TaskManager.h
  ValueConverter.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTViewer_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")


# External dependencies: OCCT libraries
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKernel TKMath TKService TKBRep TKRWMesh TKTopAlgo TKXCAF TKG3d)

# install target
OCCTViewer_install_targets(LIBRARIES ${PROJECT_NAME}
                    HEADERS ${INCFILES}
                    HEADERS_SUBDIR ${PROJECT_NAME})