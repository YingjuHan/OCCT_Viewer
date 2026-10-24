#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
ci_initialize "$@"
ci_occt_options
ci_viewer_options

# Viewer references TKViewerTest/TKDraw even with project DRAW plugins disabled.
ci_run occt-configure cmake -S "$ci_occt_source" -B "$ci_work/occt-build" -G Ninja \
    "${ci_occt_args[@]}" -DBUILD_LIBRARY_TYPE=Shared \
    "-DBUILD_ADDITIONAL_TOOLKITS=$ci_occt_toolkits;TKViewerTest" \
    -D3RDPARTY_RAPIDJSON_INCLUDE_DIR=/usr/include
ci_run occt-build cmake --build "$ci_work/occt-build" --parallel "$ci_jobs"
ci_run occt-install cmake --install "$ci_work/occt-build"

ci_run viewer-configure cmake -S "$ci_repo" -B "$ci_work/viewer" -G Ninja \
    "${ci_viewer_args[@]}" -DOCCTV_BUILD_WEB_VIEWER=OFF -DOCCTV_BUILD_TESTS=ON
ci_run viewer-build cmake --build "$ci_work/viewer" --parallel "$ci_jobs"
# Fail if no tests were registered; execute serially to bound large-fixture RAM.
export LD_LIBRARY_PATH="$ci_work/viewer/lib:$ci_work/occt-install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ci_run ctest ctest --test-dir "$ci_work/viewer" --parallel 1 --output-on-failure \
    --no-tests=error --output-junit "$ci_work/logs/ctest.xml"
