project (Viewer)

set (Viewer_BUILD_DIR ${CMAKE_BINARY_DIR}/source/${PROJECT_NAME})

set (_SRCFILES
  IManager.cpp
  InteractiveAspectsManager.cpp
  InteractiveAxesManager.cpp
  InteractiveCaeMeshManager.cpp
  NativeCutView.cpp
  NativeCutTimeline.cpp
  NativeCutAnalysis.cpp
  NativeCutAnalysisJson.cpp
  NativeCutDigest.cpp
  NativeCutSession.cpp
  InteractiveClippingManager.cpp
  InteractiveColorScaleManager.cpp
  InteractiveContext.cpp
  InteractiveDimensionManager.cpp
  InteractiveScaleRulerManager.cpp
  InteractiveShapeManager.cpp
  InteractiveTextLabelManager.cpp
  InteractiveViewCubeManager.cpp
  main.cpp
  RenderingParams.cpp
  SelectionManager.cpp
  UnitManager.cpp
  ViewManager.cpp
  WasmModulePrinter.cpp
  WasmViewer.cpp
  Utils/CaeMeshAnnotationInfo.cpp
  Utils/CaeMeshInfo.cpp
  Utils/CaeMeshPropInfo.cpp
  Utils/CaeMeshStreamlinesInfo.cpp
  Utils/CameraViewPoint.cpp
  Utils/ColorScaleParams.cpp
  Utils/DimensionParams.cpp
  Utils/SelectionMeshEntity.cpp
  Utils/SelectionShapeElems.cpp
  Utils/ShapeInfo.cpp
  Utils/VecN.cpp
)

set (_INCFILES
  IManager.h
  InteractiveAspectsManager.h
  InteractiveAxesManager.h
  InteractiveCaeMeshManager.h
  NativeCutView.h
  NativeCutTimeline.h
  NativeCutAnalysis.h
  NativeCutDigest.h
  NativeCutSession.h
  InteractiveClippingManager.h
  InteractiveColorScaleManager.h
  InteractiveContext.h
  InteractiveDimensionManager.h
  InteractiveScaleRulerManager.h
  InteractiveShapeManager.h
  InteractiveTextLabelManager.h
  InteractiveViewCubeManager.h
  RenderingParams.h
  SelectionManager.h
  UnitManager.h
  ViewManager.h
  WasmModulePrinter.h
  WasmViewer.h
  Utils/CaeMeshAnnotationInfo.h
  Utils/CaeMeshInfo.h
  Utils/CaeMeshPropInfo.h
  Utils/CaeMeshStreamlinesInfo.h
  Utils/CameraViewPoint.h
  Utils/ColorScaleParams.h
  Utils/ColorTools.h
  Utils/DimensionParams.h
  Utils/ObjectId.h
  Utils/SelectionMeshEntity.h
  Utils/SelectionShapeElems.h
  Utils/ShapeInfo.h
  Utils/VecN.h
  Utils/ViewCorner.h
  Utils/ViewCorner.lxx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../adm/cmake/OCCTViewerConfig.h.in
  ${OCCTViewer_GXX_DIR}/OCCTViewerConfig.h
)

source_group ("Source Files" FILES ${_SRCFILES})
source_group ("Header Files" FILES ${_INCFILES})
# OCCT libraries to link
set (OpenCASCADE_LIBS
     TKRWMesh TKSTL TKBinXCAF TKBin TKBinL TKXCAF TKVCAF TKCAF TKV3d TKHLR TKMesh TKService
     TKShHealing TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKLCAF TKCDF TKernel)
if (EMSCRIPTEN)
  set (OpenCASCADE_LIBS TKOpenGles ${OpenCASCADE_LIBS})
else()
  set (OpenCASCADE_LIBS TKOpenGl TKDraw TKViewerTest ${OpenCASCADE_LIBS})
endif()

# put WASM builds per configuration
if (EMSCRIPTEN)
  set (WASM_TARGET "${PROJECT_NAME}-wasm")
  set (WASM_OUTPUT_FOLDER "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/${WASM_ABI}")
endif()

# main target
if (EMSCRIPTEN)
  # extra compiler/linker flags
  set (aWasmLnkFlagsEx
    MODULARIZE=1 EXPORT_NAME='createSCwebviewer' EXPORTED_RUNTIME_METHODS=['FS','PATH'] EXPORTED_FUNCTIONS=['_main','_malloc','_free'] NO_EXIT_RUNTIME=1
  )
  foreach (aFlagIter ${aWasmLnkFlagsEx})
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ${aFlagIter}")
  endforeach()

  set (OCCTV_WASM_DEBUG_INFO "dwarf" CACHE STRING
"The mode to provide the debug info.
 - dwarf          info is embedded directly into the '.wasm' file;
 - separate-dwarf generates '.wasm' + '.debug.wasm' (-gseparate-dwarf), preferred option;
                  the debug file could be served by a deducated server
                  specified by OCCTV_WASM_DEBUG_URL (-sSEPARATE_DWARF_URL='${OCCTV_WASM_DEBUG_URL}wasm32/viewer.debug.wasm');
 - split-dwarf    generates '.wasm' + '.wasm.dwp' (-gsplit-dwarf), suboptimal;
 - source-map     generates source maps (-gsource-map) for legacy browsers;
                  the source maps could be served by a deducated server
                  specified by OCCTV_WASM_DEBUG_URL (--source-map-base='${OCCTV_WASM_DEBUG_URL}').

A browser extension 'C/C++ DevTools Support (DWARF)' could be used to debug C++ source code with 'separate-dwarf'.
https://chromewebstore.google.com/detail/cc++-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb"
  )
  set_property (CACHE OCCTV_WASM_DEBUG_INFO PROPERTY STRINGS "dwarf" "separate-dwarf" "split-dwarf" "source-map")
  # Note: Binaryen does not currently support preserving DWARF version 5 debug info. It is turned off during the Debug build, so it should be fine.
  option (OCCTV_WASM_DEBUG_USE_DWARF5 "Use DWARF version 5 with build flags that target faster debugging experience." OFF)
  if (OCCTV_WASM_DEBUG_USE_DWARF5)
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-5 -gpubnames")
  endif()

  set (OCCTV_WASM_DEBUG_URL "http://127.0.0.1:8000/" CACHE STRING
       "Base URL for debug C++ info server, where '/wasm32-pthread/SCwebviewer-wasm.debug.wasm' could be located")
  if ("${OCCTV_WASM_DEBUG_INFO}" STREQUAL "separate-dwarf")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gseparate-dwarf=${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${WASM_TARGET}.debug.wasm")
    install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${WASM_TARGET}.debug.wasm"
             DESTINATION ${WASM_OUTPUT_FOLDER} CONFIGURATIONS "Debug")

    if (NOT "${OCCTV_WASM_DEBUG_URL}" STREQUAL "")
      set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s SEPARATE_DWARF_URL=${OCCTV_WASM_DEBUG_URL}${WASM_ABI}/${WASM_TARGET}.debug.wasm")
    endif()
  elseif ("${OCCTV_WASM_DEBUG_INFO}" STREQUAL "split-dwarf")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gsplit-dwarf")
    unset (OCCTV_WASM_DEBUG_URL CACHE)
  elseif ("${OCCTV_WASM_DEBUG_INFO}" STREQUAL "source-map")
    set (CMAKE_CXX_FLAGS_DEBUG "-gsource-map -g3")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --source-map-base ${OCCTV_WASM_DEBUG_URL}")
    install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${WASM_TARGET}.wasm.map"
             DESTINATION ${WASM_OUTPUT_FOLDER} CONFIGURATIONS "Debug")
  else()
    unset (OCCTV_WASM_DEBUG_URL CACHE)
  endif()

  # target executable
  add_executable (${WASM_TARGET} ${_SRCFILES} ${_INCFILES})
  set_target_properties (${WASM_TARGET} PROPERTIES FOLDER "WebViewer")
  target_link_libraries (${WASM_TARGET} ${OpenCASCADE_LIBS} freetype)

  # produce .dwp DWARF debug package from .wasm
  if ("${OCCTV_WASM_DEBUG_INFO}" STREQUAL "split-dwarf")
    set (aDebugInfoSuffix ".wasm.dwp")
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
      find_program (DWP_BUILDER emdwp)
      add_custom_command (
        TARGET ${WASM_TARGET} POST_BUILD
        COMMAND ${DWP_BUILDER} -e ${WASM_TARGET}.wasm -o ${WASM_TARGET}${aDebugInfoSuffix}
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMENT "Linking DWARF debug package ${WASM_TARGET}${aDebugInfoSuffix}"
        VERBATIM
      )
    endif()
    install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${WASM_TARGET}${aDebugInfoSuffix}"
             DESTINATION ${WASM_OUTPUT_FOLDER} CONFIGURATIONS "Debug")
  endif()

  # internal dependencies
  target_link_libraries (${WASM_TARGET} ViewCae ViewCaeGeom ViewDim ViewShape ViewBase RWMeshConv RWShapeConv RWMed RWBase)

  # install target
  install (TARGETS "${WASM_TARGET}"
           RUNTIME DESTINATION ${WASM_OUTPUT_FOLDER}
           LIBRARY DESTINATION ${WASM_OUTPUT_FOLDER})
  install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/${WASM_TARGET}.wasm"
           DESTINATION ${WASM_OUTPUT_FOLDER}
           CONFIGURATIONS "Release")
  install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${WASM_TARGET}.wasm"
           DESTINATION ${WASM_OUTPUT_FOLDER}
           CONFIGURATIONS "Debug")
  install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO}/${WASM_TARGET}.wasm"
           DESTINATION ${WASM_OUTPUT_FOLDER}
           CONFIGURATIONS "RelWithDebInfo")

  configure_file (${CMAKE_CURRENT_SOURCE_DIR}/ts/Viewer-wasm.d.ts.in
                  ${Viewer_BUILD_DIR}/Viewer-wasm.d.ts)
  install (FILES ${Viewer_BUILD_DIR}/Viewer-wasm.d.ts
           DESTINATION "${WASM_OUTPUT_FOLDER}")

  if (OCCTV_USE_PTHREADS)
    install (FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/${WASM_TARGET}.worker.js
             DESTINATION ${WASM_OUTPUT_FOLDER}
             CONFIGURATIONS "Release" OPTIONAL)
    install (FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${WASM_TARGET}.worker.js
             DESTINATION ${WASM_OUTPUT_FOLDER}
             CONFIGURATIONS "Debug" OPTIONAL)
    install (FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO}/${WASM_TARGET}.worker.js
             DESTINATION ${WASM_OUTPUT_FOLDER}
             CONFIGURATIONS "RelWithDebInfo" OPTIONAL)
  endif()
else()
  # target library
  add_library (${PROJECT_NAME} SHARED ${_SRCFILES} ${_INCFILES})
  set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "WebViewer")

  # internal dependencies
  target_link_libraries (${PROJECT_NAME}  PUBLIC ViewCae ViewCaeGeom ViewDim ViewShape ViewBase RWMeshConv RWShapeConv RWBase RWMed)

  # OCCT and Draco libraries to link
  target_link_libraries (${PROJECT_NAME} PUBLIC ${OpenCASCADE_LIBS})

  # MEDfile library to link
  if (OCCTV_USE_MEDFILE)
    target_link_libraries (${PROJECT_NAME} PRIVATE ${MEDFILE_C_LIBRARIES})
  endif()

  # install target
  OCCTViewer_install_targets (LIBRARIES ${PROJECT_NAME}
                       HEADERS ${_INCFILES}
                       HEADERS_SUBDIR ${PROJECT_NAME})
endif()
