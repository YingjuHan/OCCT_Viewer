# This CMake module contains different platform-dependent settings of compiler, linker and etc. for Lightweight Viewer

# Compiler flags
# ==============
set (CMAKE_CXX_STANDARD 17)
if (MSVC)
  #set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /EHa /MP")
  #string (REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /MP")
  add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
  # string pooling (GF), function-level linking (Gy)
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GF /Gy")
  set (CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /GF /Gy")

  #if (BUILD_FORCE_RelWithDebInfo)
    # inline expansion level (Ob1)
    string (REGEX MATCH "Ob2" ISFLAG "${CMAKE_CXX_FLAGS_RELEASE}")
    if (ISFLAG)
      string (REGEX REPLACE "Ob2" "Ob1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
      string (REGEX REPLACE "Ob2" "Ob1" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    elseif (MSVC)
      set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ob1")
      set (CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} /Ob1")
    endif()

    # generate debug info (Zi)
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set (CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /Zi")
    # generate debug info (debug), OptimizeReferences=true (OPT:REF), EnableCOMDATFolding=true (OPT:ICF)
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    /debug /OPT:REF /OPT:ICF")
    set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /debug /OPT:REF /OPT:ICF")
  #endif()
elseif (EMSCRIPTEN)
  set (anExceptionMode "-fexceptions")
  if (OCCTV_USE_WASM_EXCEPTION)
    set (anExceptionMode "-fwasm-exceptions")
  endif()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${anExceptionMode} -fPIC -Wall -Wextra")
  add_definitions(-DOCC_CONVERT_SIGNALS)

  if (OCCTV_WARNINGS_AS_ERRORS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pthreads-mem-growth")
  endif()

  # compiler/linker flags
  set (aWasmCxxFlags)
  set (aWasmLnkFlags
       WASM=1 MAX_WEBGL_VERSION=2)
  if (OCCTV_USE_WASM_MEMORY64)
    set (aWasmLnkFlags ${aWasmLnkFlags} ALLOW_MEMORY_GROWTH=1 TOTAL_MEMORY=256MB MAXIMUM_MEMORY=8000MB)
  else()
    set (aWasmLnkFlags ${aWasmLnkFlags} ALLOW_MEMORY_GROWTH=1 TOTAL_MEMORY=256MB MAXIMUM_MEMORY=3328MB)
  endif()

  # pthread and non-pthread build option for Emscripten
  if (OCCTV_USE_PTHREADS)
    message (STATUS "PTHREADS is enabled, prepare multithread mode build")
    set (aWasmCxxFlags ${aWasmCxxFlags} -pthread)
    set (aWasmLnkFlags ${aWasmLnkFlags}
         USE_PTHREADS=1 ENVIRONMENT='web,worker'
         PTHREAD_POOL_SIZE_STRICT=0
         PTHREAD_POOL_SIZE='navigator.hardwareConcurrency-1')
    if (NOT OCCTV_USE_WASM_EXCEPTION)
      # NO_DISABLE_EXCEPTION_CATCHING is not compatible with -fwasm-exceptions (and is enabled implicitly).
      set (aWasmLnkFlags ${aWasmLnkFlags} NO_DISABLE_EXCEPTION_CATCHING)
    endif()
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
      # STACK_OVERFLOW_CHECK interferes with WASM-embedded debug info via additional Binaryen passes.
      set (aWasmLnkFlags ${aWasmLnkFlags} STACK_OVERFLOW_CHECK=1)
    endif()
  else()
    message (STATUS "PTHREADS is disabled, prepare single-thread mode build")
    set (aWasmLnkFlags ${aWasmLnkFlags}
         USE_PTHREADS=0 ENVIRONMENT='web' EXPORT_ES6=1)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    # These specific linker flags should eliminate the need for Binaryen optimization passes,
    # which do not yet have the ability to preserve most of the debugging info.
    # Moreover ERROR_ON_WASM_CHANGES_AFTER_LINK fails the build in case Binaryen tries to do any significant passes.
    set (aWasmLnkFlags ${aWasmLnkFlags} WASM_BIGINT ERROR_ON_WASM_CHANGES_AFTER_LINK REVERSE_DEPS=all ASSERTIONS=1)
  endif()

  foreach (aFlagIter ${aWasmCxxFlags})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${aFlagIter}")
  endforeach()
  foreach (aFlagIter ${aWasmLnkFlags})
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ${aFlagIter}")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s ${aFlagIter}")
  endforeach()

  if (OCCTV_USE_WASM_MEMORY64)
    # need to add to C flags in addition to C++ for CMake defining things like CMAKE_SIZEOF_VOID_P
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sMEMORY64=1")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sMEMORY64=1")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sMEMORY64=1")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -sMEMORY64=1")
  endif()

  # Link with embind
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --bind")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --bind")

  # Drop release optimization level from -O3 to -O2 to avoid CLang bugs.
  string (REGEX MATCH "-O3" IS_O3_CXX "${CMAKE_CXX_FLAGS_RELEASE}")
  if (IS_O3_CXX)
    string (REGEX REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  else()
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
  endif()

else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fPIC")
  add_definitions(-DOCC_CONVERT_SIGNALS)

  if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "[Cc][Ll][Aa][Nn][Gg]"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  endif()

  if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # -Wconversion is stronger check than -Wfloat-conversion
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfloat-conversion -Wunused -Wshadow=local")
  endif()

  if (OCCTV_WARNINGS_AS_ERRORS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif()

  # -dynamicbase/-pie linker flag for executables
  set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -pie")

  #set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-z,defs")
  #set (CMAKE_SHARED_LINKER_FLAGS_DEBUG   "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}   -Wl,-z,defs")

  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-multiple-definition")
endif()

# Release/RelWithDebInfo/Debug build directories
# ==============================================
set (BIN_LETTER "")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set (BIN_LETTER "d")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
  set (BIN_LETTER "i")
endif()
if (OCCTV_SINGLE_GENERATOR)
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib${BIN_LETTER}")
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin${BIN_LETTER}")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib${BIN_LETTER}")
  if (WIN32)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin${BIN_LETTER}")
  endif()
endif()

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin")

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/libd")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bind")

if (WIN32)
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/bin")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/bind")
else()
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/lib")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/lib")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/libd")
endif()

# Install target directories for Release/Debug
# ============================================
set (OCCTV_INSTALL_INCLUDE      include)
set (OCCTV_INSTALL_BIN_RELEASE  bin)
set (OCCTV_INSTALL_BIN_DEBUG    bind)
set (OCCTV_INSTALL_LIB_RELEASE  lib)
set (OCCTV_INSTALL_LIB_DEBUG    libd)
set (OCCTV_INSTALL_DLL_DEBUG    ${OCCTV_INSTALL_LIB_DEBUG})
SET (OCCTV_INSTALL_CMAKE_CONFIG lib/cmake/${OCCTViewer_PACKAGE})
if (WIN32)
  set (OCCTV_INSTALL_DLL_RELEASE ${OCCTV_INSTALL_BIN_RELEASE})
  set (OCCTV_INSTALL_DLL_DEBUG   ${OCCTV_INSTALL_BIN_DEBUG})
endif()

# Library type
# ============
if (EMSCRIPTEN)
  set (OCCTV_LIBTYPE STATIC)
else()
  set (OCCTV_LIBTYPE SHARED)
endif()

# Wasm ABI suffix
# ===============
if (EMSCRIPTEN)
  set (WASM_ABI "wasm32")
  if (OCCTV_USE_WASM_MEMORY64)
    set (WASM_ABI "wasm64")
  endif()
  if (OCCTV_USE_PTHREADS)
    set (WASM_ABI "${WASM_ABI}-pthread")
  endif()
endif()

