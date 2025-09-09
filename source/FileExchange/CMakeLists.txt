project (FileExchange)

# med to mdb convertor
add_executable("${PROJECT_NAME}" FileExchange.cpp)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Tools")

# internal dependencies
target_link_libraries (${PROJECT_NAME} PRIVATE RWMeshConv)

# OCCT dependencies
target_link_libraries (${PROJECT_NAME} PRIVATE TKernel)

# install executable
OCCTViewer_install_targets(EXECUTABLES ${PROJECT_NAME})
