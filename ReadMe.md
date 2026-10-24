# OCC_VIEWER

Web viewer built on OpenCASCADE (OCCT) with WebAssembly.

## Web Support Matrix
Shape (default):
- .glb/.gltf
- .brep/.bbrep/.binbrep/.sbrep
- .stl

Shape (optional):
- .step/.stp (OCCTV_USE_STEP=ON)
- .iges/.igs (OCCTV_USE_IGES=ON)

CAE mesh:
- .pos, .msh (Gmsh)
- .mdb (mesh binary)

## Web Build Flags
- OCCTV_BUILD_WEB_VIEWER=ON
- OCCTV_USE_STEP=ON/OFF
- OCCTV_USE_IGES=ON/OFF
- OCCTV_USE_DRACO=ON/OFF (glTF Draco)
- OCCTV_USE_PTHREADS=ON/OFF (wasm multithreading)
- OCCTV_USE_WASM_EXCEPTION=ON/OFF
- OCCTV_USE_WASM_MEMORY64=ON/OFF
- OCCTV_WASM_CONFIGS (e.g. "wasm32-pthread wasm32")

## Notes
- STEP/IGES direct read is optional; if disabled, use server-side conversion to glb/metadata.
- MED (.med/.rmed) requires OCCTV_USE_MEDFILE and is desktop-only (non-EMSCRIPTEN).
