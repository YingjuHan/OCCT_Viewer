# Repository Guidelines

## Project Structure & Module Organization

This repository builds an OpenCASCADE (OCCT) CAD/CAE viewer for desktop and WebAssembly. C++ libraries live in `source/`, grouped by responsibility (`RWCae`, `ViewCae`, `Viewer`, and others). `source-draw/` contains DRAW Harness commands and plugins; `source-ts/Viewer-ts/` contains the browser API. Browser examples are in `samples/`, including the Vue app in `samples/vue-viewer/`. Keep fonts and example models in `resources/`; `test/cae/` holds DRAW test scripts. `install/` contains packaged WebAssembly builds and sample copies, so make source changes in the corresponding `source*` or `samples/` directory first.

## Build, Test, and Development Commands

Install CMake, Ninja, a C++ compiler, OCCT, RapidJSON, and the OpenGL/X11 development libraries before a Linux build. The CI workflow uses this reduced native configuration:

```bash
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DOCCTV_BUILD_DRAW=OFF -DOCCTV_BUILD_WEB_VIEWER=OFF \
  -DOCCTV_USE_DRACO=OFF -DOCCTV_USE_MEDFILE=OFF \
  -DOCCTV_INSTALL_DEPENDENCIES=OFF
cmake --build build/dev --parallel
```

For the browser sample, run `cd samples/vue-viewer && npm ci && npm run serve`; `npm run build` creates its production bundle. The sample depends on the local package in `install/wasm32-fallback`. WebAssembly builds require Emscripten and an OCCT WebAssembly SDK; see `.github/workflows/ci-build.yml` for the toolchain and CMake flags.

## Coding Style & Naming Conventions

Follow the style of the module you edit. C++ uses four-space indentation, PascalCase types, and matching `.h`/`.cpp` filenames; existing DRAW commands commonly use `Scv` prefixes. The TypeScript API uses two-space indentation and PascalCase filenames for exported classes. No repository-wide formatter or linter configuration is present, so keep formatting changes local to your work.

## Testing Guidelines

The CI workflow builds native and WebAssembly targets, but does not register CTest cases or define a coverage threshold. `test/cae/color_map` is a DRAW script; put similar cases under `test/<area>/` with descriptive snake_case names and run them in a configured DRAW Harness environment. For viewer changes, also build the Vue sample and check the affected interaction in a browser. There is no `npm test` script in the sample.

## Commit & Pull Request Guidelines

Recent commits use short, descriptive subjects, sometimes prefixed `feat:`, `fix:`, or `docs:`. Keep each commit focused. In a pull request, explain the behavior changed, link the relevant issue when applicable, list the build or manual checks performed, and include screenshots for visible UI changes.

## Agent-Specific Instructions

For structural codebase exploration, use the installed `codebase-memory` skill and verify source directly when its index reports coverage gaps.
