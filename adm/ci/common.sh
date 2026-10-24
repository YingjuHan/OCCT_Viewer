#!/usr/bin/env bash
# Sourced by the CI entry points; invoke entry points with bash on every platform.
set -euo pipefail

ci_initialize() {
    if [[ $# != 2 ]]; then
        echo "Usage: bash ${0##*/} /path/to/OCCT_private /path/to/work-directory" >&2
        return 2
    fi
    ci_repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
    ci_occt_source=$(cd -- "$1" && pwd)
    mkdir -p -- "$2"
    ci_work=$(cd -- "$2" && pwd)
    ci_jobs=${CI_BUILD_JOBS:-2}
    if [[ ! $ci_jobs =~ ^[1-9][0-9]*$ ]]; then
        echo 'CI_BUILD_JOBS must be a positive integer' >&2
        return 2
    fi
    if [[ ! -f "$ci_occt_source/src/AIS/AIS_ClippingPlanes.hxx" ]]; then
        echo 'Matching OCCT_private/ViewCaeGeom source is required (AIS_ClippingPlanes.hxx missing)' >&2
        return 2
    fi
    mkdir -p "$ci_work/logs"
    {
        date -u +%FT%TZ
        cmake --version | head -1
        printf 'viewer_revision '
        git -C "$ci_repo" rev-parse HEAD
        printf 'occt_revision '
        if [[ -e "$ci_occt_source/.git" ]]; then
            git -C "$ci_occt_source" rev-parse HEAD
        else
            printf '%s\n' "${CI_OCCT_REVISION:-source-archive}"
        fi
        printf 'build_jobs %s\n' "$ci_jobs"
    } | tee "$ci_work/logs/versions.txt"
}

ci_run() {
    local name=$1
    shift
    "$@" 2>&1 | tee "$ci_work/logs/$name.log"
}

# OCCT computes transitive toolkit dependencies. Select project-required leaves.
ci_occt_options() {
    ci_occt_args=(
        -DCMAKE_BUILD_TYPE=Release
        "-DINSTALL_DIR=$ci_work/occt-install"
        -DINSTALL_DIR_LAYOUT=Unix
        -DINSTALL_DIR_WITH_VERSION=OFF
        -DINSTALL_DIR_LIB=lib
        -DINSTALL_DIR_CMAKE=lib/cmake/opencascade
        -DBUILD_MODULE_FoundationClasses=OFF
        -DBUILD_MODULE_ModelingData=OFF
        -DBUILD_MODULE_ModelingAlgorithms=OFF
        -DBUILD_MODULE_Visualization=OFF
        -DBUILD_MODULE_ApplicationFramework=OFF
        -DBUILD_MODULE_DataExchange=OFF
        -DBUILD_MODULE_Draw=OFF
        -DBUILD_DOC_Overview=OFF
        -DBUILD_SAMPLES_QT=OFF
        -DBUILD_Inspector=OFF
        -DUSE_TBB=OFF
        -DUSE_VTK=OFF
        -DUSE_FREEIMAGE=OFF
        -DUSE_FFMPEG=OFF
        -DUSE_DRACO=OFF
        -DUSE_RAPIDJSON=ON
    )
    ci_occt_toolkits='TKBO;TKBinXCAF;TKRWMesh;TKSTL;TKVCAF;TKHLR;TKMesh;TKShHealing;TKPrim'
}

ci_viewer_options() {
    ci_viewer_args=(
        -DCMAKE_BUILD_TYPE=Release
        "-DOpenCASCADE_ROOT_DIR=$ci_work/occt-install"
        -DOCCTV_BUILD_DRAW=OFF
        -DOCCTV_BUILD_MESH_EXCHANGER=OFF
        -DOCCTV_INSTALL_DEPENDENCIES=OFF
        -DOCCTV_USE_CONAN=OFF
        -DOCCTV_USE_DRACO=OFF
        -DOCCTV_USE_MEDFILE=OFF
    )
}
