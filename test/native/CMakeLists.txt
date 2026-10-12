find_package (ZLIB REQUIRED)
add_executable (occtviewer_tests VtkTests.cpp)
target_link_libraries (occtviewer_tests PRIVATE RWVtk RWMeshConv ViewCaeGeom ZLIB::ZLIB)
target_compile_definitions (occtviewer_tests PRIVATE VTK_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/")
add_test (NAME vtk_reader COMMAND occtviewer_tests)
add_executable (vtk_format_tests VtkFormatTests.cpp)
target_link_libraries (vtk_format_tests PRIVATE RWMeshConv)
target_compile_definitions (vtk_format_tests PRIVATE VTK_FORMAT_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/vtk-format-fixtures")
add_test (NAME vtk_format COMMAND vtk_format_tests)
set_tests_properties (vtk_format PROPERTIES TIMEOUT 60)
add_executable (vtk_series_tests VtkSeriesTests.cpp)
target_link_libraries (vtk_series_tests PRIVATE RWMeshConv)
target_compile_definitions (vtk_series_tests PRIVATE VTK_SERIES_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/vtk-series-fixtures")
add_test (NAME vtk_series COMMAND vtk_series_tests)
set_tests_properties (vtk_series PROPERTIES TIMEOUT 60 FIXTURES_SETUP vtk_series_fixtures)
add_executable (vtk_safety_tests VtkSafetyTests.cpp)
target_link_libraries (vtk_safety_tests PRIVATE RWMeshConv ZLIB::ZLIB)
add_test (NAME vtk_safety COMMAND vtk_safety_tests)
set_tests_properties (vtk_safety PROPERTIES TIMEOUT 60)
add_executable (vtk_limits_tests VtkLimitsTests.cpp)
target_link_libraries (vtk_limits_tests PRIVATE RWMeshConv)
if (WIN32)
  target_link_libraries (vtk_limits_tests PRIVATE psapi)
endif()
target_compile_definitions (vtk_limits_tests PRIVATE VTK_LIMIT_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/vtk-limit-fixtures" VTK_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test (NAME vtk_limits COMMAND vtk_limits_tests)
set_tests_properties (vtk_limits PROPERTIES TIMEOUT 60)
add_executable (vtk_reference_tests VtkReferenceTests.cpp)
target_link_libraries (vtk_reference_tests PRIVATE RWMeshConv)
if (TARGET Viewer)
  target_link_libraries (vtk_reference_tests PRIVATE Viewer)
  target_compile_definitions (vtk_reference_tests PRIVATE OCCTV_TEST_VIEWER)
endif()
target_compile_definitions (vtk_reference_tests PRIVATE VTK_REFERENCE_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/vtk-reference-fixtures")
add_test (NAME vtk_reference COMMAND vtk_reference_tests)
set_tests_properties (vtk_reference PROPERTIES FIXTURES_SETUP vtk_reference_fixtures TIMEOUT 60
  PASS_REGULAR_EXPRESSION "VTK_REFERENCE_PASS" FAIL_REGULAR_EXPRESSION "VTK_REFERENCE_FAIL")
add_executable (vtk_import_tests VtkImportTests.cpp)
target_link_libraries (vtk_import_tests PRIVATE RWMeshConv)
if (TARGET Viewer)
  target_link_libraries (vtk_import_tests PRIVATE Viewer)
  target_compile_definitions (vtk_import_tests PRIVATE OCCTV_TEST_VIEWER)
endif()
target_compile_definitions (vtk_import_tests PRIVATE VTK_IMPORT_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/vtk-import-fixtures" VTK_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test (NAME vtk_import COMMAND vtk_import_tests)
set_tests_properties (vtk_import PROPERTIES FIXTURES_SETUP vtk_import_fixtures TIMEOUT 60
  PASS_REGULAR_EXPRESSION "VTK_IMPORT_PASS" FAIL_REGULAR_EXPRESSION "VTK_IMPORT_FAIL")
add_executable (cut_view_tests CutViewTests.cpp)
target_link_libraries (cut_view_tests PRIVATE ViewCaeGeom ViewCae)
add_test (NAME cut_view COMMAND cut_view_tests)
add_executable (cut_display_tests CutDisplayTests.cpp)
target_link_libraries (cut_display_tests PRIVATE ViewCae)
add_test (NAME cut_display COMMAND cut_display_tests)
add_executable (cut_slicing_tests CutSlicingTests.cpp)
target_link_libraries (cut_slicing_tests PRIVATE ViewCaeGeom)
add_test (NAME cut_slicing COMMAND cut_slicing_tests)
add_executable (cut_cache_tests CutCacheTests.cpp)
target_link_libraries (cut_cache_tests PRIVATE ViewCaeGeom)
add_test (NAME cut_cache COMMAND cut_cache_tests)
add_executable (cut_physics_tests CutPhysicsTests.cpp)
target_link_libraries (cut_physics_tests PRIVATE ViewCaeGeom)
add_test (NAME cut_physics COMMAND cut_physics_tests)
set_tests_properties (cut_physics PROPERTIES TIMEOUT 90)
if (TARGET Viewer)
  add_executable (native_cut_tests NativeCutTests.cpp)
  target_link_libraries (native_cut_tests PRIVATE Viewer)
  target_compile_definitions (native_cut_tests PRIVATE NATIVE_CUT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
  add_test (NAME native_cut COMMAND native_cut_tests)
  add_executable (native_timeline_tests NativeTimelineTests.cpp)
  target_link_libraries (native_timeline_tests PRIVATE Viewer)
  add_test (NAME native_timeline COMMAND native_timeline_tests)
  add_executable (native_analysis_tests NativeAnalysisTests.cpp)
  target_link_libraries (native_analysis_tests PRIVATE Viewer)
  target_compile_definitions (native_analysis_tests PRIVATE NATIVE_CUT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
  add_test (NAME native_analysis COMMAND native_analysis_tests)
endif()
add_executable (quadric_cut_tests QuadricCutTests.cpp)
target_link_libraries (quadric_cut_tests PRIVATE ViewCaeGeom ViewCae)
add_test (NAME quadric_cut COMMAND quadric_cut_tests)
add_executable (cut_recovery_tests CutRecoveryTests.cpp)
target_link_libraries (cut_recovery_tests PRIVATE ViewCaeGeom)
add_test (NAME cut_recovery COMMAND cut_recovery_tests)
add_executable (cut_tensor_tests CutTensorTests.cpp)
target_link_libraries (cut_tensor_tests PRIVATE ViewCaeGeom ViewCae)
add_test (NAME cut_tensor COMMAND cut_tensor_tests)
add_executable (cut_trace_binding_tests CutTraceBindingTests.cpp)
target_link_libraries (cut_trace_binding_tests PRIVATE ViewCaeGeom)
add_test (NAME cut_trace_binding COMMAND cut_trace_binding_tests)
add_executable (cut_union_tests CutUnionTests.cpp)
target_link_libraries (cut_union_tests PRIVATE ViewCaeGeom ViewCae)
add_test (NAME cut_union COMMAND cut_union_tests)
set_tests_properties (cut_union PROPERTIES TIMEOUT 300)
if (OCCTV_USE_VTK)
  add_executable (cut_interface_tests CutInterfaceTests.cpp)
  target_link_libraries (cut_interface_tests PRIVATE ViewCaeGeom)
  add_test (NAME cut_interface COMMAND cut_interface_tests)
  set_tests_properties (cut_interface PROPERTIES TIMEOUT 180)
  add_executable (vtk_cell_cut_tests VtkCellCutTests.cpp)
  target_link_libraries (vtk_cell_cut_tests PRIVATE RWVtk ViewCaeGeom ViewCae)
  target_compile_definitions (vtk_cell_cut_tests PRIVATE VTK_CELL_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/vtk-cell-fixtures")
  add_test (NAME vtk_cell_cut COMMAND vtk_cell_cut_tests)
  add_executable (general_cut_cache_tests GeneralCutCacheTests.cpp)
  target_link_libraries (general_cut_cache_tests PRIVATE RWVtk ViewCaeGeom)
  target_compile_definitions (general_cut_cache_tests PRIVATE GENERAL_CACHE_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/general-cache-fixtures")
  add_test (NAME general_cut_cache COMMAND general_cut_cache_tests)
  set_tests_properties (general_cut_cache PROPERTIES TIMEOUT 120 FIXTURES_SETUP general_cut_cache_fixtures)
  add_executable (cut_geometry_tests CutGeometryTests.cpp)
  target_link_libraries (cut_geometry_tests PRIVATE RWVtk ViewCaeGeom)
  target_compile_definitions (cut_geometry_tests PRIVATE CUT_GEOMETRY_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/cut-geometry-fixtures")
  add_test (NAME cut_geometry COMMAND cut_geometry_tests)
  set_tests_properties (cut_geometry PROPERTIES TIMEOUT 300 FIXTURES_SETUP cut_geometry_fixtures)
  add_executable (cut_analytic_tests CutAnalyticTests.cpp)
  target_link_libraries (cut_analytic_tests PRIVATE RWVtk ViewCaeGeom)
  target_compile_definitions (cut_analytic_tests PRIVATE CUT_ANALYTIC_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/cut-analytic-fixtures")
  add_test (NAME cut_analytic COMMAND cut_analytic_tests)
  set_tests_properties (cut_analytic PROPERTIES TIMEOUT 180 FIXTURES_SETUP cut_analytic_fixtures)
  add_executable (cut_reference_tests CutReferenceTests.cpp)
  target_link_libraries (cut_reference_tests PRIVATE RWVtk ViewCaeGeom)
  target_compile_definitions (cut_reference_tests PRIVATE CUT_REFERENCE_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/cut-reference-fixtures")
  add_test (NAME cut_reference COMMAND cut_reference_tests)
  set_tests_properties (cut_reference PROPERTIES TIMEOUT 180 FIXTURES_SETUP cut_reference_fixtures)
  add_executable (cut_iso_tests CutIsoTests.cpp)
  target_link_libraries (cut_iso_tests PRIVATE RWVtk ViewCaeGeom ViewCae)
  target_compile_definitions (cut_iso_tests PRIVATE CUT_ISO_FIXTURE_DIR="${CMAKE_CURRENT_BINARY_DIR}/cut-iso-fixtures")
  add_test (NAME cut_iso COMMAND cut_iso_tests)
  set_tests_properties (cut_iso PROPERTIES TIMEOUT 240 FIXTURES_SETUP cut_iso_fixtures)
  set (VTK_NATIVE_TESTS cut_interface vtk_cell_cut general_cut_cache cut_geometry cut_analytic cut_reference cut_iso)
  set_tests_properties (vtk_cell_cut PROPERTIES TIMEOUT 300 FIXTURES_SETUP vtk_cell_fixtures)
  set (OCCTV_VTK_TEST_DATA "" CACHE PATH "Optional VTK ExternalData/Testing/Data directory")
  foreach (fixture quadraticTetra01.vtu onePolyhedron.vtu concavePolyhedron.vtu nonplanar_quad.vtk Disc_BiQuadraticQuads_0_0.vtu cube.vtu)
    if (EXISTS "${OCCTV_VTK_TEST_DATA}/${fixture}")
      get_filename_component (fixture_name "${fixture}" NAME_WE)
      add_test (NAME "vtk_external_${fixture_name}" COMMAND vtk_cell_cut_tests --external "${OCCTV_VTK_TEST_DATA}/${fixture}")
      list (APPEND VTK_NATIVE_TESTS "vtk_external_${fixture_name}")
      set_tests_properties ("vtk_external_${fixture_name}" PROPERTIES TIMEOUT 120)
    endif()
  endforeach()
  if (EXISTS "${OCCTV_VTK_TEST_DATA}/BoxIncorrectConnectivity.vtp")
    add_test (NAME vtk_invalid_connectivity COMMAND vtk_cell_cut_tests --reject-import "${OCCTV_VTK_TEST_DATA}/BoxIncorrectConnectivity.vtp")
    list (APPEND VTK_NATIVE_TESTS vtk_invalid_connectivity)
  endif()
  if (EXISTS "${OCCTV_VTK_TEST_DATA}/nonWatertightPolyhedron.vtu")
    add_test (NAME vtk_nonwatertight COMMAND vtk_cell_cut_tests --reject-cut "${OCCTV_VTK_TEST_DATA}/nonWatertightPolyhedron.vtu")
    list (APPEND VTK_NATIVE_TESTS vtk_nonwatertight)
  endif()
  if (UNIX)
    set_tests_properties (${VTK_NATIVE_TESTS} PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:ViewCaeGeom>:${OpenCASCADE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
  endif()
endif()
add_executable (cut_view_benchmark CutViewBenchmark.cpp)
target_link_libraries (cut_view_benchmark PRIVATE ViewCaeGeom)
if (UNIX)
  set_tests_properties (vtk_reader vtk_format vtk_series vtk_safety vtk_limits vtk_reference vtk_import cut_view quadric_cut cut_recovery cut_tensor cut_trace_binding PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:ViewCaeGeom>:${OpenCASCADE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
endif()

if (TARGET ViewDraw)
  find_program (OCCTV_DRAWEXE NAMES DRAWEXE HINTS "${OpenCASCADE_BINARY_DIR}")
  if (OCCTV_DRAWEXE)
    add_test (NAME draw_cut_view COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view.tcl")
    set (DRAW_TEST_ENV
      "CSF_OCCTResourcePath=${OpenCASCADE_RESOURCE_DIR}"
      "CSF_ShadersDirectory=${OpenCASCADE_RESOURCE_DIR}/Shaders"
      "CSF_DrawPluginDefaults=${OpenCASCADE_RESOURCE_DIR}/DrawResources"
      "DRAWHOME=${OpenCASCADE_RESOURCE_DIR}/DrawResources"
      "DRAWDEFAULT=${OpenCASCADE_RESOURCE_DIR}/DrawResources/DrawDefault"
      "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:ViewDraw>:${OpenCASCADE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
    add_test (NAME draw_vtk_import COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/vtk_import.tcl")
    set_tests_properties (draw_vtk_import PROPERTIES
      ENVIRONMENT "${DRAW_TEST_ENV};VTK_IMPORT_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-import-fixtures;VTK_IMPORT_HAS_NATIVE=$<BOOL:${OCCTV_USE_VTK}>"
      FIXTURES_REQUIRED vtk_import_fixtures TIMEOUT 60
      PASS_REGULAR_EXPRESSION "VTK_IMPORT_DRAW_PASS" FAIL_REGULAR_EXPRESSION "VTK_IMPORT_DRAW_FAIL")
    add_test (NAME draw_vtk_reference COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/vtk_references.tcl")
    set_tests_properties (draw_vtk_reference PROPERTIES
      ENVIRONMENT "${DRAW_TEST_ENV};VTK_REFERENCE_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-reference-fixtures;VTK_REFERENCE_HAS_NATIVE=$<BOOL:${OCCTV_USE_VTK}>"
      FIXTURES_REQUIRED vtk_reference_fixtures TIMEOUT 90
      PASS_REGULAR_EXPRESSION "VTK_REFERENCE_DRAW_PASS" FAIL_REGULAR_EXPRESSION "VTK_REFERENCE_DRAW_FAIL")
    set_tests_properties (draw_cut_view PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_DRAW_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_DRAW_FAIL")
    add_test (NAME draw_cut_view_iso COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_iso.tcl")
    set_tests_properties (draw_cut_view_iso PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_ISO_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
    add_test (NAME draw_cut_view_tensor COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_tensor.tcl")
    add_test (NAME draw_cut_view_trace COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_trace.tcl")
    add_test (NAME draw_cut_view_union COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_union.tcl")
    set_tests_properties (draw_cut_view_union PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_UNION_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 120)
    set_tests_properties (draw_cut_view_trace PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_TRACE_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
    set_tests_properties (draw_cut_view_tensor PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_TENSOR_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
    add_test (NAME draw_cut_view_quadric COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_quadric.tcl")
    set_tests_properties (draw_cut_view_quadric PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_QUADRIC_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
    if (OCCTV_USE_VTK)
      add_test (NAME draw_cut_view_high_order_iso COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_high_order_iso.tcl")
      set_tests_properties (draw_cut_view_high_order_iso PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_ISO_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-iso-fixtures"
        FIXTURES_REQUIRED cut_iso_fixtures TIMEOUT 120
        PASS_REGULAR_EXPRESSION "CUT_VIEW_HIGH_ORDER_ISO_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
      add_test (NAME draw_cut_view_references COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_references.tcl")
      set_tests_properties (draw_cut_view_references PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_REFERENCE_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-reference-fixtures"
        FIXTURES_REQUIRED cut_reference_fixtures TIMEOUT 120
        PASS_REGULAR_EXPRESSION "CUT_VIEW_REFERENCE_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_REFERENCE_FAIL")
      add_test (NAME draw_cut_view_analytic COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_analytic.tcl")
      set_tests_properties (draw_cut_view_analytic PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_ANALYTIC_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-analytic-fixtures"
        FIXTURES_REQUIRED cut_analytic_fixtures TIMEOUT 120
        PASS_REGULAR_EXPRESSION "CUT_VIEW_ANALYTIC_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
      add_test (NAME draw_cut_view_geometry COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_geometry.tcl")
      set_tests_properties (draw_cut_view_geometry PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_GEOMETRY_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-geometry-fixtures"
        FIXTURES_REQUIRED cut_geometry_fixtures TIMEOUT 120
        PASS_REGULAR_EXPRESSION "CUT_VIEW_GEOMETRY_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
      add_test (NAME draw_cut_view_cache COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_cache.tcl")
      set_tests_properties (draw_cut_view_cache PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_CACHE_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/general-cache-fixtures"
        FIXTURES_REQUIRED general_cut_cache_fixtures TIMEOUT 60
        PASS_REGULAR_EXPRESSION "CUT_VIEW_CACHE_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
      add_test (NAME draw_cut_view_vtk_cells COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_vtk_cells.tcl")
      set_tests_properties (draw_cut_view_vtk_cells PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_VTK_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-cell-fixtures"
        FIXTURES_REQUIRED vtk_cell_fixtures TIMEOUT 300
        PASS_REGULAR_EXPRESSION "CUT_VIEW_VTK_CELLS_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
    endif()
    option (OCCTV_TEST_DRAW_VISUAL "Run DRAW image regressions (requires a working X display)" OFF)
    add_test (NAME draw_cut_view_recovery COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_recovery.tcl")
    set_tests_properties (draw_cut_view_recovery PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_RECOVERY_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 90)
    if (OCCTV_TEST_DRAW_VISUAL)
      add_test (NAME draw_cut_view_benchmark_smoke COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_benchmark.tcl")
      set_tests_properties (draw_cut_view_benchmark_smoke PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_BENCHMARK_GRID=6;CUTVIEW_BENCHMARK_FRAMES=3;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-benchmark-smoke"
        RUN_SERIAL TRUE TIMEOUT 90 PASS_REGULAR_EXPRESSION "CUT_BENCHMARK_PASS" FAIL_REGULAR_EXPRESSION "CUT_BENCHMARK_FAIL")
      add_test (NAME draw_cut_view_physics COMMAND "${OCCTV_DRAWEXE}" -i -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_physics_test.tcl")
      set_tests_properties (draw_cut_view_physics PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-physics"
        RUN_SERIAL TRUE TIMEOUT 90 PASS_REGULAR_EXPRESSION "CUT_VIEW_PHYSICS_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
      add_test (NAME draw_vtk_reference_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/vtk_references.tcl")
      set_tests_properties (draw_vtk_reference_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};VTK_REFERENCE_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-reference-fixtures;VTK_REFERENCE_HAS_NATIVE=$<BOOL:${OCCTV_USE_VTK}>;VTK_REFERENCE_VISUAL=1;VTK_REFERENCE_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/draw-vtk-references"
        FIXTURES_REQUIRED vtk_reference_fixtures RUN_SERIAL TRUE TIMEOUT 120
        PASS_REGULAR_EXPRESSION "VTK_REFERENCE_DRAW_PASS" FAIL_REGULAR_EXPRESSION "VTK_REFERENCE_DRAW_FAIL")
      add_test (NAME draw_vtk_import_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/vtk_import.tcl")
      set_tests_properties (draw_vtk_import_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};VTK_IMPORT_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-import-fixtures;VTK_IMPORT_HAS_NATIVE=$<BOOL:${OCCTV_USE_VTK}>;VTK_IMPORT_VISUAL=1;VTK_IMPORT_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/draw-import"
        FIXTURES_REQUIRED vtk_import_fixtures RUN_SERIAL TRUE TIMEOUT 90
        PASS_REGULAR_EXPRESSION "VTK_IMPORT_DRAW_PASS" FAIL_REGULAR_EXPRESSION "VTK_IMPORT_DRAW_FAIL")
      add_test (NAME draw_cut_view_recovery_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_recovery.tcl")
      set_tests_properties (draw_cut_view_recovery_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_RECOVERY_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-recovery"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_RECOVERY_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 90)
      if (OCCTV_USE_VTK)
        add_test (NAME draw_cut_view_high_order_iso_visual COMMAND "${OCCTV_DRAWEXE}" -i -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_high_order_iso.tcl")
        set_tests_properties (draw_cut_view_high_order_iso_visual PROPERTIES
          ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_ISO_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-iso-fixtures;CUTVIEW_HIGH_ISO_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-high-iso"
          FIXTURES_REQUIRED cut_iso_fixtures RUN_SERIAL TRUE TIMEOUT 180
          PASS_REGULAR_EXPRESSION "CUT_VIEW_HIGH_ORDER_ISO_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
        add_test (NAME draw_cut_view_references_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_references.tcl")
        set_tests_properties (draw_cut_view_references_visual PROPERTIES
          ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_REFERENCE_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-reference-fixtures;CUTVIEW_REFERENCE_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-references"
          FIXTURES_REQUIRED cut_reference_fixtures RUN_SERIAL TRUE TIMEOUT 180
          PASS_REGULAR_EXPRESSION "CUT_VIEW_REFERENCE_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_REFERENCE_FAIL")
        add_test (NAME draw_cut_view_analytic_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_analytic.tcl")
        set_tests_properties (draw_cut_view_analytic_visual PROPERTIES
          ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_ANALYTIC_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-analytic-fixtures;CUTVIEW_ANALYTIC_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-analytic"
          FIXTURES_REQUIRED cut_analytic_fixtures RUN_SERIAL TRUE TIMEOUT 180
          PASS_REGULAR_EXPRESSION "CUT_VIEW_ANALYTIC_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
        add_test (NAME draw_cut_view_geometry_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_geometry.tcl")
        set_tests_properties (draw_cut_view_geometry_visual PROPERTIES
          ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_GEOMETRY_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/cut-geometry-fixtures;CUTVIEW_GEOMETRY_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-geometry"
          FIXTURES_REQUIRED cut_geometry_fixtures RUN_SERIAL TRUE TIMEOUT 180
          PASS_REGULAR_EXPRESSION "CUT_VIEW_GEOMETRY_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
        add_test (NAME draw_cut_view_cache_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_cache.tcl")
        set_tests_properties (draw_cut_view_cache_visual PROPERTIES
          ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_CACHE_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/general-cache-fixtures;CUTVIEW_CACHE_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-cache"
          FIXTURES_REQUIRED general_cut_cache_fixtures RUN_SERIAL TRUE TIMEOUT 60
          PASS_REGULAR_EXPRESSION "CUT_VIEW_CACHE_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
        add_test (NAME draw_cut_view_vtk_cells_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_vtk_cells.tcl")
        set_tests_properties (draw_cut_view_vtk_cells_visual PROPERTIES
          ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_VTK_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-cell-fixtures;CUTVIEW_VTK_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-vtk-cells"
          FIXTURES_REQUIRED vtk_cell_fixtures RUN_SERIAL TRUE TIMEOUT 300
          PASS_REGULAR_EXPRESSION "CUT_VIEW_VTK_CELLS_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL")
      endif()
      add_test (NAME draw_cut_view_quadric_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_quadric.tcl")
      set_tests_properties (draw_cut_view_quadric_visual PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_QUADRIC_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-quadric"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_QUADRIC_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 120)
      add_test (NAME draw_cut_view_controls COMMAND "${OCCTV_DRAWEXE}" -i -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_controls.tcl")
      add_test (NAME draw_cut_view_tools COMMAND "${OCCTV_DRAWEXE}" -i -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_tools_test.tcl")
      add_test (NAME draw_cut_view_time COMMAND "${OCCTV_DRAWEXE}" -i -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_time_test.tcl")
      set_tests_properties (draw_cut_view_time PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-time;VTK_SERIES_FIXTURES=${CMAKE_CURRENT_BINARY_DIR}/vtk-series-fixtures"
        FIXTURES_REQUIRED vtk_series_fixtures PASS_REGULAR_EXPRESSION "CUT_VIEW_TIME_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 120)
      set_tests_properties (draw_cut_view_tools PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-tools"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_TOOLS_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 120)
      add_test (NAME draw_cut_view_tensor_visual COMMAND "${OCCTV_DRAWEXE}" -i -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_tensor.tcl")
      add_test (NAME draw_cut_view_trace_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_trace.tcl")
      add_test (NAME draw_cut_view_union_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_union.tcl")
      set_tests_properties (draw_cut_view_union_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_UNION_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-union"
        RUN_SERIAL TRUE PASS_REGULAR_EXPRESSION "CUT_VIEW_UNION_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 180)
      set_tests_properties (draw_cut_view_trace_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_TRACE_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-trace"
        RUN_SERIAL TRUE PASS_REGULAR_EXPRESSION "CUT_VIEW_TRACE_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
      set_tests_properties (draw_cut_view_tensor_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_TENSOR_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-tensor"
        RUN_SERIAL TRUE PASS_REGULAR_EXPRESSION "CUT_VIEW_TENSOR_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
      set_tests_properties (draw_cut_view_controls PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-controls"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_CONTROLS_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 120)
      if (WIN32)
        set_tests_properties (draw_cut_view_controls PROPERTIES TIMEOUT 300)
      endif()
      add_test (NAME draw_cut_view_iso_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_iso.tcl")
      set_tests_properties (draw_cut_view_iso_visual PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_ISO_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-iso-images"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_ISO_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 60)
      add_test (NAME draw_cut_view_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_visual.tcl")
      set_tests_properties (draw_cut_view_visual PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-images"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_VISUAL_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE)
      add_test (NAME draw_cut_view_interaction COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_interaction.tcl")
      set_tests_properties (draw_cut_view_interaction PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-interaction"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_INTERACTION_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 60)
    endif()
  endif()
endif()

# Windows loads shared OCCT/VTK and compiler runtimes from PATH, not LD_LIBRARY_PATH.
if (WIN32)
  get_filename_component (OCCTV_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component (OCCTV_ZLIB_LIB_DIR "${ZLIB_LIBRARY}" DIRECTORY)
  set (OCCTV_TEST_PATH "$<TARGET_FILE_DIR:ViewCaeGeom>;${OpenCASCADE_BINARY_DIR};${OCCTV_COMPILER_BIN};${OCCTV_ZLIB_LIB_DIR}/../bin")
  if (OCCTV_USE_VTK)
    string (APPEND OCCTV_TEST_PATH ";$<TARGET_FILE_DIR:VTK::CommonCore>")
  endif()
  string (APPEND OCCTV_TEST_PATH ";$ENV{PATH}")
  string (REPLACE ";" "\\;" OCCTV_TEST_PATH "${OCCTV_TEST_PATH}")
  get_property (OCCTV_REGISTERED_TESTS DIRECTORY PROPERTY TESTS)
  set_property (TEST ${OCCTV_REGISTERED_TESTS} APPEND PROPERTY ENVIRONMENT "PATH=${OCCTV_TEST_PATH}")
endif()
