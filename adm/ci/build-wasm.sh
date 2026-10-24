#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
ci_initialize "$@"
: "${EMSDK:?Activate Emscripten 3.1.58 before running this script}"
command -v emcc >/dev/null
command -v tsc >/dev/null
ci_occt_options
ci_viewer_options
ci_toolchain="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
{ emcc --version; tsc --version; } | tee -a "$ci_work/logs/versions.txt"

# Keep host libc headers out of Emscripten's include search path.
mkdir -p "$ci_work/rapidjson/include"
cp -a /usr/include/rapidjson "$ci_work/rapidjson/include/"
ci_freetype_version=2.13.2
ci_run freetype-download curl --fail --location --retry 3 --show-error --silent \
    --connect-timeout 20 --max-time 180 \
    "https://github.com/freetype/freetype/archive/refs/tags/VER-${ci_freetype_version//./-}.tar.gz" \
    -o "$ci_work/freetype.tar.gz"
mkdir -p "$ci_work/freetype-source"
tar -xf "$ci_work/freetype.tar.gz" -C "$ci_work/freetype-source" --strip-components=1
ci_run freetype-configure cmake -S "$ci_work/freetype-source" -B "$ci_work/freetype-build" -G Ninja \
    "-DCMAKE_TOOLCHAIN_FILE=$ci_toolchain" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    "-DCMAKE_INSTALL_PREFIX=$ci_work/freetype-install" \
    -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON \
    -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
ci_run freetype-build cmake --build "$ci_work/freetype-build" --parallel "$ci_jobs"
ci_run freetype-install cmake --install "$ci_work/freetype-build"

ci_run zlib-port embuilder build zlib
ci_sysroot="$EMSDK/upstream/emscripten/cache/sysroot"
ci_zlib="$ci_sysroot/lib/wasm32-emscripten/libz.a"
test -f "$ci_zlib"

ci_run occt-configure cmake -S "$ci_occt_source" -B "$ci_work/occt-build" -G Ninja \
    "${ci_occt_args[@]}" "-DCMAKE_TOOLCHAIN_FILE=$ci_toolchain" -DBUILD_LIBRARY_TYPE=Static \
    "-DBUILD_ADDITIONAL_TOOLKITS=$ci_occt_toolkits;TKOpenGles" \
    "-D3RDPARTY_FREETYPE_DIR=$ci_work/freetype-install" \
    "-D3RDPARTY_FREETYPE_INCLUDE_DIR_ft2build=$ci_work/freetype-install/include/freetype2" \
    "-D3RDPARTY_FREETYPE_INCLUDE_DIR_freetype2=$ci_work/freetype-install/include/freetype2" \
    "-D3RDPARTY_FREETYPE_LIBRARY=$ci_work/freetype-install/lib/libfreetype.a" \
    "-D3RDPARTY_RAPIDJSON_INCLUDE_DIR=$ci_work/rapidjson/include"
ci_run occt-build cmake --build "$ci_work/occt-build" --parallel "$ci_jobs"
ci_run occt-install cmake --install "$ci_work/occt-build"

ci_run viewer-configure cmake -S "$ci_repo" -B "$ci_work/viewer" -G Ninja \
    "${ci_viewer_args[@]}" "-DCMAKE_TOOLCHAIN_FILE=$ci_toolchain" \
    "-DFREETYPE_ROOT_DIR=$ci_work/freetype-install" "-DRapidJSON_ROOT_DIR=$ci_work/rapidjson" \
    "-DZLIB_INCLUDE_DIR=$ci_sysroot/include" "-DZLIB_LIBRARY=$ci_zlib" \
    -DCMAKE_C_FLAGS=-sUSE_ZLIB=1 -DCMAKE_CXX_FLAGS=-sUSE_ZLIB=1 \
    -DOCCTV_BUILD_TESTS=OFF -DOCCTV_BUILD_WEB_VIEWER=ON \
    -DOCCTV_USE_PTHREADS=OFF -DOCCTV_USE_WASM_EXCEPTION=OFF -DOCCTV_USE_WASM_MEMORY64=OFF \
    -DOCCTV_WASM_CONFIGS=wasm32 "-DOCCTV_WEBVIEWER_INSTALL_PREFIX=$ci_work/viewer-install"
ci_run viewer-build cmake --build "$ci_work/viewer" --parallel "$ci_jobs"
ci_run viewer-install cmake --install "$ci_work/viewer"
test -s "$ci_work/viewer-install/wasm32/Viewer-wasm.wasm"
test -s "$ci_work/viewer-install/wasm32/Viewer-wasm.js"
echo CI_WASM_BUILD_PASS | tee "$ci_work/logs/result.txt"
