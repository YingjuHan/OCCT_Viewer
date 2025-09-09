project (RWMed)

set (SRCFILES
  RWMedReader.cpp
)
set (INCFILES
  RWMedReader.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

# library to build
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PUBLIC RWBase RWCae)

# OCCT libraries to link
target_link_libraries (${PROJECT_NAME}
                       PUBLIC TKernel TKMath TKService TKBRep TKGeomBase TKV3d TKXCAF TKG3d TKTopAlgo)

# MED libraries to link
if (OCCTV_USE_MEDFILE)
  target_link_libraries (${PROJECT_NAME} PRIVATE ${MEDFILE_C_LIBRARIES})
endif()

# install target
OCCTViewer_install_targets(LIBRARIES ${PROJECT_NAME}
                    HEADERS ${INCFILES}
                    HEADERS_SUBDIR ${PROJECT_NAME})