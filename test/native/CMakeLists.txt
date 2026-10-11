find_package (ZLIB REQUIRED)
add_executable (occtviewer_tests VtkTests.cpp)
target_link_libraries (occtviewer_tests PRIVATE RWVtk RWMeshConv ViewCaeGeom ZLIB::ZLIB)
target_compile_definitions (occtviewer_tests PRIVATE VTK_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/")
add_test (NAME vtk_reader COMMAND occtviewer_tests)
add_executable (cut_view_tests CutViewTests.cpp)
target_link_libraries (cut_view_tests PRIVATE ViewCaeGeom ViewCae)
add_test (NAME cut_view COMMAND cut_view_tests)
add_executable (quadric_cut_tests QuadricCutTests.cpp)
target_link_libraries (quadric_cut_tests PRIVATE ViewCaeGeom ViewCae)
add_test (NAME quadric_cut COMMAND quadric_cut_tests)
add_executable (cut_recovery_tests CutRecoveryTests.cpp)
target_link_libraries (cut_recovery_tests PRIVATE ViewCaeGeom)
add_test (NAME cut_recovery COMMAND cut_recovery_tests)
if (OCCTV_USE_VTK)
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
  set (VTK_NATIVE_TESTS vtk_cell_cut general_cut_cache cut_geometry cut_analytic)
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
  set_tests_properties (vtk_reader cut_view quadric_cut cut_recovery PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${OpenCASCADE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
endif()

if (TARGET ViewDraw)
  find_program (OCCTV_DRAWEXE NAMES DRAWEXE HINTS "${OpenCASCADE_BINARY_DIR}")
  if (OCCTV_DRAWEXE)
    add_test (NAME draw_cut_view COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view.tcl")
    set (DRAW_TEST_ENV
      "CSF_OCCTResourcePath=${OpenCASCADE_RESOURCE_DIR}"
      "DRAWHOME=${OpenCASCADE_RESOURCE_DIR}/DrawResources"
      "DRAWDEFAULT=${OpenCASCADE_RESOURCE_DIR}/DrawResources/DrawDefault"
      "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:ViewDraw>:${OpenCASCADE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
    set_tests_properties (draw_cut_view PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_DRAW_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_DRAW_FAIL")
    add_test (NAME draw_cut_view_iso COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_iso.tcl")
    set_tests_properties (draw_cut_view_iso PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_ISO_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
    add_test (NAME draw_cut_view_quadric COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_quadric.tcl")
    set_tests_properties (draw_cut_view_quadric PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV}"
      PASS_REGULAR_EXPRESSION "CUT_VIEW_QUADRIC_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" TIMEOUT 60)
    if (OCCTV_USE_VTK)
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
      add_test (NAME draw_cut_view_recovery_visual COMMAND "${OCCTV_DRAWEXE}" -b -f "${PROJECT_SOURCE_DIR}/test/cae/cut_view_recovery.tcl")
      set_tests_properties (draw_cut_view_recovery_visual PROPERTIES
        ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_RECOVERY_VISUAL=1;CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-recovery"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_RECOVERY_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 90)
      if (OCCTV_USE_VTK)
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
      set_tests_properties (draw_cut_view_controls PROPERTIES ENVIRONMENT "${DRAW_TEST_ENV};CUTVIEW_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/draw-controls"
        PASS_REGULAR_EXPRESSION "CUT_VIEW_CONTROLS_PASS" FAIL_REGULAR_EXPRESSION "CUT_VIEW_.*FAIL" RUN_SERIAL TRUE TIMEOUT 120)
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
