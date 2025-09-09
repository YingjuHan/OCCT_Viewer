project (RWMeshExchanger)

set (CMAKE_CXX_STANDARD 17)

# Library sources
set (SRCFILES
  MeshExchanger.cpp
  Smds2MeshDataConvert.cpp
  MeshExchangerUtils.cpp
  MedFileUtils.cpp
)

set (INCFILES
  MeshExchanger.h
  Smds2MeshDataConvert.h
  MeshExchangerUtils.h
  MedFileUtils.h
)

source_group ("Source Files" FILES ${SRCFILES})
source_group ("Header Files" FILES ${INCFILES})

add_subdirectory (DriverBDF)
add_subdirectory (DriverINP)

# Add Library
add_library (${PROJECT_NAME} ${OCCTV_LIBTYPE} ${SRCFILES} ${INCFILES})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

target_link_libraries (${PROJECT_NAME} PUBLIC
  ${SMESH_LIBRARIES}
  RWBase RWCae RWMeshConv RWShape
  MeshDriverCGNS MeshDriverBDF MeshDriverINP
  ${OpenCASCADE_FoundationClasses_LIBRARIES}
  ${OpenCASCADE_ModelingData_LIBRARIES}
  ${OpenCASCADE_ModelingAlgorithms_LIBRARIES})
target_include_directories (${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${MEDFILE_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${SMESH_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
if (MINGW)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

# Add executable
if (OCCTV_BUILD_MESH_EXCHANGER_EXE)
  add_executable(${PROJECT_NAME}EXE MeshExchangerEXE.cpp)
  target_link_libraries(${PROJECT_NAME}EXE ${PROJECT_NAME})
  # install executable
  lwv_install_targets(EXECUTABLES ${PROJECT_NAME}EXE)
endif()

# install target and header
lwv_install_targets (LIBRARIES ${PROJECT_NAME}
                     HEADERS ${INCFILES}
                     HEADERS_SUBDIR ${PROJECT_NAME})