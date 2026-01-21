project (Viewer-ts)

set (ViewerTS_BUILD_DIR ${CMAKE_BINARY_DIR}/source-ts/${PROJECT_NAME})
set (Viewer_PACKAGE "Viewer")

set (Viewer_TSFILES
  Camera.ts
  CommonTools.ts
  InteractiveCaeMesh.ts
  InteractiveCaeMeshManager.ts
  InteractiveClipping.ts
  InteractiveColorScale.ts
  InteractiveDimension.ts
  InteractiveObject.ts
  InteractiveScaleRuler.ts
  InteractiveShape.ts
  MeasurementStore.ts
  PmiStore.ts
  ProjectStore.ts
  SavedViewStore.ts
  SceneGraph.ts
  InteractiveTextLabel.ts
  InteractiveViewCube.ts
  InteractiveSphericalAxes.ts
  SCwebviewer.ts
  Subview.ts
  ViewerEventManager.ts
  ViewerEventTarget.ts
  ViewManager.ts
)
set (Viewer_TSFILES_CFG
  ViewerBuildConfig.ts
)
set (Viewer_CONFIGFILES
  cmake/tsconfig.json.in
  cmake/tsconfig-build-config.json.in
  cmake/package.json.in
)
source_group ("Source Files" FILES ${Viewer_TSFILES} ${Viewer_TSFILES_CFG})
source_group ("Config Files" FILES ${Viewer_CONFIGFILES})

# generate JavaScript files from TypeScript
if (EMSCRIPTEN)
  # Emsdk includes node.js, so that tsc compiler can be installed via:
  # > emcmdprompt.bat
  # > npm install -g typescript
  # After that, tsc.cmd will be in PATH after calling emsdk_env.bat.
  set (TSC_EXECUTABLE "tsc")
  if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    set (TSC_EXECUTABLE "tsc.cmd")
  endif()

  # generate tsconfig.json
  set (TSC_CONFIG ${ViewerTS_BUILD_DIR}/ts/tsconfig.json)
  configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/tsconfig.json.in ${TSC_CONFIG})

  set (TSC_INPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/tsconfig.json.in
    ${Viewer_TSFILES} ${Viewer_TSFILES_CFG})

  set (TSC_OUTPUT)
  foreach (aTsFile ${Viewer_TSFILES})
    get_filename_component (aTsFileBase ${aTsFile} NAME_WE)
    list (APPEND TSC_OUTPUT "${ViewerTS_BUILD_DIR}/js/${aTsFileBase}.js")
    list (APPEND TSC_OUTPUT "${ViewerTS_BUILD_DIR}/js/${aTsFileBase}.d.ts")
    list (APPEND TSC_OUTPUT "${ViewerTS_BUILD_DIR}/js/${aTsFileBase}.js.map")
  endforeach()

  add_custom_command (OUTPUT ${TSC_OUTPUT}
    COMMAND "${TSC_EXECUTABLE}" --build "${TSC_CONFIG}"
    DEPENDS ${TSC_INPUT}
    COMMENT "Calling TypeScript compiler ${TSC_EXECUTABLE}..."
    VERBATIM)
  add_custom_target (tsc ALL DEPENDS ${TSC_OUTPUT})
  set_target_properties (tsc PROPERTIES FOLDER "WebViewer")

  install (FILES ${TSC_OUTPUT}
           DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/wasm32")
  install (FILES ${Viewer_TSFILES}
           DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/source-ts/Viewer-ts")

  # Generate 'ViewerBuildConfig.ts' with WASM configurations defined by OCCTV_WASM_CONFIGS option.
  # Generated file is compiled by tsc as a separate building step
  # to allow IDE and typedoc to work transparently with the main source code and simplify its 'tsconfig.json'.
  set (VIEWER_BUILDCFG_IN  ${CMAKE_CURRENT_SOURCE_DIR}/ViewerBuildConfig.ts)
  set (VIEWER_BUILDCFG_OUT ${ViewerTS_BUILD_DIR}/ts-gen/ViewerBuildConfig.ts)
  set (VIEWER_BUILDCFG_TSC_CONFIG_IN  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/tsconfig-build-config.json.in)
  set (VIEWER_BUILDCFG_TSC_CONFIG_OUT ${ViewerTS_BUILD_DIR}/ts-gen/tsconfig-build-config.json)
  configure_file (${VIEWER_BUILDCFG_IN} ${VIEWER_BUILDCFG_OUT})
  configure_file (${VIEWER_BUILDCFG_TSC_CONFIG_IN} ${VIEWER_BUILDCFG_TSC_CONFIG_OUT})
  set (VIEWER_BUILDCFG_TSC_INPUT ${VIEWER_BUILDCFG_TSC_CONFIG_IN} ${VIEWER_BUILDCFG_IN})
  set (VIEWER_BUILDCFG_TSC_OUTPUT)
  foreach (aTsFile ${VIEWER_BUILDCFG_IN})
    get_filename_component (aTsFileBase ${aTsFile} NAME_WE)
    list (APPEND VIEWER_BUILDCFG_TSC_OUTPUT "${ViewerTS_BUILD_DIR}/js-gen/${aTsFileBase}.js")
    list (APPEND VIEWER_BUILDCFG_TSC_OUTPUT "${ViewerTS_BUILD_DIR}/js-gen/${aTsFileBase}.d.ts")
    list (APPEND VIEWER_BUILDCFG_TSC_OUTPUT "${ViewerTS_BUILD_DIR}/js-gen/${aTsFileBase}.js.map")
  endforeach()

  add_custom_command (OUTPUT ${VIEWER_BUILDCFG_TSC_OUTPUT}
    COMMAND "${TSC_EXECUTABLE}" --build "${VIEWER_BUILDCFG_TSC_CONFIG_OUT}"
    DEPENDS ${VIEWER_BUILDCFG_TSC_INPUT}
    COMMENT "Calling TypeScript compiler ${TSC_EXECUTABLE}..."
    VERBATIM)
  add_custom_target (tsc-cfg ALL DEPENDS ${VIEWER_BUILDCFG_TSC_OUTPUT})
  set_target_properties (tsc-cfg PROPERTIES FOLDER "WebViewer")

  install (FILES ${VIEWER_BUILDCFG_TSC_OUTPUT}
           DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/wasm32")
  install (FILES ${VIEWER_BUILDCFG_OUT}
           DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}/source-ts/SCwebviewer-ts")
else()
  add_custom_target (${PROJECT_NAME} SOURCES ${Viewer_TSFILES} ${Viewer_CONFIGFILES})
  set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "WebViewer")
endif()

# TypeDoc target
if (EMSCRIPTEN)
  set (OCCTViewer_BUILD_DOC_TYPESCRIPT OFF CACHE BOOL "Build TypeScript documentation")
else()
  set (OCCTViewer_BUILD_DOC_TYPESCRIPT OFF)
endif()
if (OCCTViewer_BUILD_DOC_TYPESCRIPT)
  find_program (TYPEDOC_EXE "typedoc")
  if (TYPEDOC_EXE)
    set (DOC_TYPESCRIPT_OUT ${ViewerTS_BUILD_DIR}/doc)

    set (toFailTypeDocOnWarning false)
    if (OCCTV_WARNINGS_AS_ERRORS)
      set (toFailTypeDocOnWarning true)
    endif()
    add_custom_command (DEPENDS ${TSC_INPUT}
                        COMMAND ${TYPEDOC_EXE}
                        --tsconfig "${TSC_CONFIG}"
                        --plugin "typedoc-plugin-markdown"
                        --out "${DOC_TYPESCRIPT_OUT}"
                        --treatWarningsAsErrors ${toFailTypeDocOnWarning}
                        "${CMAKE_CURRENT_SOURCE_DIR}/SCwebviewer.ts"
                        COMMENT "TypeScript documentation generation at '${DOC_TYPESCRIPT_OUT}'"
                        OUTPUT "${DOC_TYPESCRIPT_OUT}"
                        VERBATIM)
    add_custom_target (doc-ts ALL DEPENDS ${DOC_TYPESCRIPT_OUT})
    set_target_properties (doc-ts PROPERTIES FOLDER "Documentation")
    install (DIRECTORY ${DOC_TYPESCRIPT_OUT} DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}")
  else()
    message (SEND_ERROR "TypeDoc program not found. You can install it by 'npm i typedoc typedoc-plugin-markdown -g' command")
  endif()
endif()

# NPM module target - package.json generation
if (EMSCRIPTEN)
  configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/package.json.in ${ViewerTS_BUILD_DIR}/package.json NEWLINE_STYLE LF)
  message(${ViewerTS_BUILD_DIR})
  install (FILES ${ViewerTS_BUILD_DIR}/package.json DESTINATION "${OCCTV_WEBVIEWER_INSTALL_PREFIX}")
endif()
