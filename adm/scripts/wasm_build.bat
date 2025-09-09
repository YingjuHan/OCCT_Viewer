@echo OFF

rem Auxiliary script for semi-automated building of WebGL viewer.
rem wasm_custom.bat should be configured with paths to CMake, 3rd-parties and Emscripten SDK.

set "OCC_VIEWER_GIT_ROOT=%~dp0..\.."
set "OCC_VIEWER_SRC_ROOT=%~dp0..\.."
set "OCC_VIEWER_BUILD_ROOT=%OCC_VIEWER_GIT_ROOT%\work"

set OCC_VIEWER_NB_JOBS=%NUMBER_OF_PROCESSORS%

rem Paths to 3rd-party tools and libraries
set "OpenCASCADE_DIR=C:\Users\hanyi\workspace\code_repo\OCCT\OGG_2024.4\install\wasm64-fallback-fwasm-exceptions"
set "OCC_VIEWER_CMAKE_BIN=C:\Program Files\CMake\bin\cmake.exe"
set "OCC_VIEWER_GENERATOR=MinGW Makefiles"
@REM set "OCC_VIEWER_GENERATOR=Ninja"
set "TYPEDOC_EXE=C:\Users\hanyi\AppData\Roaming\npm\typedoc.cmd"
set "FREETYPE_DIR="
set "RAPIDJSON_DIR="
set "DRACO_DIR="

rem Build stages to perform
set "OCC_VIEWER_DO_CMAKE=1"
set "OCC_VIEWER_DO_CLEAN=0"
set "OCC_VIEWER_DO_MAKE=1"
set "OCC_VIEWER_DO_INSTALL=1"
set "OCC_VIEWER_WASM_DEBUG_INFO=dwarf"
set "OCC_VIEWER_WASM_DEBUG_URL="
set "OCC_VIEWER_WASM_DEBUG_USE_DWARF5=OFF"

set "OCC_VIEWER_BUILD_TYPE=Release"
set "OCC_VIEWER_WARNINGS_AS_ERRORS=OFF"
set "OCC_VIEWER_BUILD_DOC_TYPESCRIPT=OFF"
set "OCC_VIEWER_USE_DRACO=ON"

rem WebAssembly exception handling mode
set "OCC_VIEWER_USE_WASM_EXCEPTION="

set "OCC_VIEWER_BUILD_WEB_VIEWER=ON"

set "OCC_VIEWER_BUILD_FOLDER_NAME=occ_viewer"

rem WASM configurations to build in preferred loading order
set "OCC_VIEWER_WASM_CONFIGS=wasm32-pthread wasm32"
rem set "OCC_VIEWER_WASM_CONFIGS=wasm64-pthread wasm64 wasm32-pthread wasm32"

rem Configuration file
if exist "%~dp0wasm_custom.bat" call "%~dp0wasm_custom.bat %"

call "%EMSDK_ROOT%\emsdk_env.bat"
set "OCC_VIEWER_TOOLCHAIN=%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
if not ["%OCC_VIEWER_CMAKE_BIN%"] == [""] ( set "PATH=%OCC_VIEWER_CMAKE_BIN%;%PATH%" )

set "OCC_VIEWER_BUILD_TYPE_PREFIX="
if /I not ["%OCC_VIEWER_BUILD_TYPE%"] == ["Release"] (
  set "OCC_VIEWER_BUILD_TYPE_PREFIX=-%OCC_VIEWER_BUILD_TYPE%"
)

set "OCC_VIEWER_WEB_DEST_DIR=%OCC_VIEWER_BUILD_ROOT%\%OCC_VIEWER_BUILD_FOLDER_NAME%-wasm%OCC_VIEWER_BUILD_TYPE_PREFIX%"
if /I ["%OCC_VIEWER_BUILD_WEB_VIEWER%"] == ["ON"] (
  if ["%OCC_VIEWER_DO_CMAKE%"] == ["1"] (
    if ["%OCC_VIEWER_DO_CLEAN%"] == ["1"] (
      if exist "%OCC_VIEWER_WEB_DEST_DIR%" (
        rem rmdir /S /Q "%OCC_VIEWER_WEB_DEST_DIR%"
        for /f "tokens=*" %%i in ('DIR /a:d /b %OCC_VIEWER_WEB_DEST_DIR%') DO (
          if not ["%%i"] == [".git"] ( rmdir /S /Q "%OCC_VIEWER_WEB_DEST_DIR%\%%i" )
        )
        for /f "tokens=*" %%i in ('DIR /a:-d /b %OCC_VIEWER_WEB_DEST_DIR%') DO (
          del "%OCC_VIEWER_WEB_DEST_DIR%\%%i"
        )
      )
    )
  )
  xcopy /S /Y "%OCC_VIEWER_SRC_ROOT%\SCwebviewer\.gitattributes" "%OCC_VIEWER_WEB_DEST_DIR%\"
  xcopy /S /Y "%OCC_VIEWER_SRC_ROOT%\SCwebviewer\.gitignore"     "%OCC_VIEWER_WEB_DEST_DIR%\"
)

for %%s in (%OCC_VIEWER_WASM_CONFIGS%) do (
  call :cmakeGenerate "%%s"
  if errorlevel 1 (
    if not ["%1"] == ["-nopause"] ( pause )
    exit /B 1
    goto :eof
  )
)
if not ["%1"] == ["-nopause"] ( pause )
goto :eof

:cmakeGenerate
set "OCC_VIEWER_WASM_ABI=%~1"
if ["%OCC_VIEWER_WASM_ABI%"] == [""] ( set "OCC_VIEWER_WASM_ABI=wasm32" )

set "OCC_VIEWER_WASM_ABI_SUFFIX=-%OCC_VIEWER_WASM_ABI%"
set "OCC_VIEWER_USE_PTHREADS=OFF"
set "OCC_VIEWER_USE_WASM_MEMORY64=OFF"
set "OCC_VIEWER_USE_WASM_EXCEPTION=%OCC_VIEWER_USE_WASM_EXCEPTION%"
set "OCC_VIEWER_EXTRA_CFLAGS="
if ["%OCC_VIEWER_USE_WASM_EXCEPTION%"] == [""] (
  rem Enable for all builds except fallback wasm32 by default
  set "OCC_VIEWER_USE_WASM_EXCEPTION=ON"
  if /I ["%OCC_VIEWER_WASM_ABI%"] == ["wasm32"] set "OCC_VIEWER_USE_WASM_EXCEPTION=OFF"
)
if /I ["%OCC_VIEWER_WASM_ABI%"] == ["wasm32-pthread"] (
  set "OCC_VIEWER_USE_PTHREADS=ON"
)
if /I ["%OCC_VIEWER_WASM_ABI%"] == ["wasm64-pthread"] (
  set "OCC_VIEWER_USE_WASM_MEMORY64=ON"
  set "OCC_VIEWER_USE_PTHREADS=ON"
  set "OCC_VIEWER_EXTRA_CFLAGS=-sMEMORY64=1"
)
if /I ["%OCC_VIEWER_WASM_ABI%"] == ["wasm64"] (
  set "OCC_VIEWER_USE_WASM_MEMORY64=ON"
  set "OCC_VIEWER_EXTRA_CFLAGS=-sMEMORY64=1"
)

set "OCC_VIEWER_DEST_DIR=%OCC_VIEWER_BUILD_ROOT%\%OCC_VIEWER_BUILD_FOLDER_NAME%%OCC_VIEWER_WASM_ABI_SUFFIX%"

if ["%OCC_VIEWER_DO_CMAKE%"] == ["1"] (
  if ["%OCC_VIEWER_DO_CLEAN%"] == ["1"] (
    if exist "%OCC_VIEWER_DEST_DIR%" (
      rmdir /S /Q "%OCC_VIEWER_DEST_DIR%"
    )
  )
)

set "OCC_VIEWER_WORK_DIR=%OCC_VIEWER_BUILD_ROOT%\%OCC_VIEWER_BUILD_FOLDER_NAME%%OCC_VIEWER_WASM_ABI_SUFFIX%%OCC_VIEWER_BUILD_TYPE_PREFIX%-make"
set "OCC_VIEWER_LOG_FILE=%OCC_VIEWER_BUILD_ROOT%\%OCC_VIEWER_BUILD_FOLDER_NAME%%OCC_VIEWER_WASM_ABI_SUFFIX%%OCC_VIEWER_BUILD_TYPE_PREFIX%-build.log"
if ["%OCC_VIEWER_DO_CMAKE%"] == ["1"] (
  if ["%OCC_VIEWER_DO_CLEAN%"] == ["1"] (
    rmdir /S /Q "%OCC_VIEWER_WORK_DIR%"
  )
)
if not exist "%OCC_VIEWER_WORK_DIR%" ( mkdir "%OCC_VIEWER_WORK_DIR%" )
if     exist "%OCC_VIEWER_LOG_FILE%" ( del   "%OCC_VIEWER_LOG_FILE%" )
echo "%OpenCASCADE_DIR%%OCC_VIEWER_WASM_ABI_SUFFIX%/lib/cmake/opencascade"
if ["%OCC_VIEWER_DO_CMAKE%"] == ["1"] (
  cmake -G "%OCC_VIEWER_GENERATOR%" ^
 -D CMAKE_TOOLCHAIN_FILE:FILEPATH="%OCC_VIEWER_TOOLCHAIN%" ^
 -D CMAKE_BUILD_TYPE:STRING="%OCC_VIEWER_BUILD_TYPE%" ^
 -D CMAKE_INSTALL_PREFIX:PATH="%OCC_VIEWER_DEST_DIR%" ^
 -D CMAKE_C_FLAGS="%OCC_VIEWER_EXTRA_CFLAGS%" ^
 -D CMAKE_CXX_FLAGS="%OCC_VIEWER_EXTRA_CFLAGS%" ^
 -D OCC_VIEWER_WASM_DEBUG_INFO:STRING="%OCC_VIEWER_WASM_DEBUG_INFO%" ^
 -D OCC_VIEWER_WASM_DEBUG_URL:STRING="%OCC_VIEWER_WASM_DEBUG_URL%" ^
 -D OCC_VIEWER_WASM_DEBUG_USE_DWARF5:BOOL="%OCC_VIEWER_WASM_DEBUG_USE_DWARF5%" ^
 -D OCC_VIEWER_WARNINGS_AS_ERRORS:BOOL="%OCC_VIEWER_WARNINGS_AS_ERRORS%" ^
 -D OPENCASCADE_ROOT_DIR:PATH="%OPENCASCADE_DIR%%OCC_VIEWER_WASM_ABI_SUFFIX%" ^
 -D FREETYPE_ROOT_DIR:PATH="%FREETYPE_DIR%%OCC_VIEWER_WASM_ABI_SUFFIX%" ^
 -D RAPIDJSON_ROOT_DIR:PATH="%RAPIDJSON_DIR%" ^
 -D OCC_VIEWER_USE_DRACO:BOOL="%OCC_VIEWER_USE_DRACO%" ^
 -D DRACO_ROOT_DIR:PATH="%DRACO_DIR%%OCC_VIEWER_WASM_ABI_SUFFIX%" ^
 -D OCC_VIEWER_USE_PTHREADS:BOOL="%OCC_VIEWER_USE_PTHREADS%" ^
 -D OCC_VIEWER_BUILD_DOC_TYPESCRIPT:BOOL="%OCC_VIEWER_BUILD_DOC_TYPESCRIPT%" ^
 -D TYPEDOC_EXE:PATH="%TYPEDOC_EXE%" ^
 -D OCC_VIEWER_USE_WASM_EXCEPTION:BOOL="%OCC_VIEWER_USE_WASM_EXCEPTION%" ^
 -D OCC_VIEWER_USE_WASM_MEMORY64:BOOL="%OCC_VIEWER_USE_WASM_MEMORY64%" ^
 -D OCC_VIEWER_WASM_CONFIGS:STRING="%OCC_VIEWER_WASM_CONFIGS%" ^
 -D OCC_VIEWER_BUILD_WEB_VIEWER:BOOL="%OCC_VIEWER_BUILD_WEB_VIEWER%" ^
 -D OCC_VIEWER_WEBVIEWER_INSTALL_PREFIX:PATH="%OCC_VIEWER_WEB_DEST_DIR%" ^
 -S "%OCC_VIEWER_SRC_ROOT%" -B "%OCC_VIEWER_WORK_DIR%"

  if errorlevel 1 (
    exit /B 1
    goto :eof
  )
)
if ["%OCC_VIEWER_DO_CLEAN%"] == ["1"] (
  cmake --build "%OCC_VIEWER_WORK_DIR%" --target clean
)

if ["%OCC_VIEWER_DO_MAKE%"] == ["1"] (
  echo Building...
  cmake.exe --build "%OCC_VIEWER_WORK_DIR%" --target all -- -j %OCC_VIEWER_NB_JOBS% 2>> "%OCC_VIEWER_LOG_FILE%"
  if errorlevel 1 (
    type "%OCC_VIEWER_LOG_FILE%"
    exit /B 1
    goto :eof
  )
  type "%OCC_VIEWER_LOG_FILE%"
)

if /I ["%OCC_VIEWER_BUILD_DOC_TYPESCRIPT%"] == ["ON"] (
  cmake --build "%OCC_VIEWER_WORK_DIR%" --target doc-ts 2>> "%OCC_VIEWER_LOG_FILE%"
  if errorlevel 1 (
    type "%OCC_VIEWER_LOG_FILE%"
    exit /B 1
    goto :eof
  )
)

if ["%OCC_VIEWER_DO_INSTALL%"] == ["1"] (
  cmake --build "%OCC_VIEWER_WORK_DIR%" --target install 2>> "%OCC_VIEWER_LOG_FILE%"
  if errorlevel 1 (
    type "%OCC_VIEWER_LOG_FILE%"
    exit /B 1
    goto :eof
  )
)

goto :eof