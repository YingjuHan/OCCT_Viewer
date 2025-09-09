# OCC_VIEWER Architecture

## 1. Overview
- OCC_VIEWER builds on OpenCASCADE (OCCT) to visualize both B-Rep geometry and CAE meshes in browsers via WebAssembly.
- The WebAssembly runtime is `Viewer::WasmViewer` (source/Viewer/WasmViewer.h:54) with TypeScript bindings declared in source/Viewer/ts/Viewer-wasm.d.ts.in.
- This document complements source/readme.md with layering, dependencies, and data/control flows.

## 2. Tech Stack & Directory Map
| Layer | Key Paths | Notes |
| --- | --- | --- |
| Presentation | source/Viewer, source/ViewBase | Wasm runtime, managers, Emscripten hooks |
| Visualization | source/ViewShape, source/ViewCae, source/ViewCaeGeom, source/ViewDim | AIS presentations, effects, dimensions |
| Data & Processing | source/RWShape, source/RWCae, source/RWMeshConv, source/RWMeshExchanger, source/RWMed, source/RWVtk, source/LodCae, source/SimMesh | Geometry/mesh models, converters, simplifiers |
| Infrastructure | source/RWBase, source/FileExchange, source/Viewer/Utils | Tasking, promises, virtual FS, CLI |

## 3. Layered View
See `DOCUMENT/diagrams/architecture-layered.drawio` for the visual version.
1. Presentation & Web Integration: `Viewer::WasmViewer`, `ViewManager`, `SelectionManager`, TS declarations.
2. Visualization Services: `ViewBase::IInteractiveObject` (source/ViewBase/IInteractiveObject.h:19), `ViewShape::InteractiveShape`, `ViewCae::InteractiveCaeMesh` (source/ViewCae/InteractiveCaeMesh.cpp:44), `ViewDim::*`.
3. Data Domain & Processing: `RWShape::ShapeData` (source/RWShape/ShapeData.h:44), `RWCae::MeshInfo` (source/RWCae/MeshInfo.h:10), `RWMeshConv::MedConverter` (source/RWMeshConv/MedConverter.h:24), `LodCae::Simplifier` (source/LodCae/Simplifier.cpp:28).
4. Platform & Infrastructure: `RWBase::TaskManager` (source/RWBase/TaskManager.h:15), `EmulatedFileSystem` (source/RWBase/EmulatedFileSystem.h:10), `FileSystemMap` (source/RWBase/FileSystemMap.h:10), `FileExchange` CLI (source/FileExchange/FileExchange.cpp:1).

## 4. Module Responsibilities
### 4.1 Infrastructure (RWBase & Utils)
- `RWBase::TaskManager` offers main/background queues and parallel loops; pairs with `RWBase::ConditionPromise` (source/RWBase/ConditionPromise.h:6) to coordinate Wasm main-thread execution.
- `RWBase::EmulatedFileSystem` plus `RWBase::FileSystemMap` virtualize file access so the TS side can mount in-memory content.
- `RWBase::IPromise` (source/RWBase/IPromise.h:5) and `_cppWasmCallBack` bridge TS Promises with C++ callbacks.
- `Viewer::Utils` supplies cross-module primitives (`ObjectId`, `VecN`, `ColorTools`, `SelectionMeshEntity`) and Emscripten bindings.

### 4.2 Data Domain & Conversion
- B-Rep: `RWShape::ShapeData` stores root shapes, sub-shapes, metadata, visibility and stats; `ViewShape::InteractiveShapeOwner` links them to OCCT owners.
- CAE Mesh: `RWCae::MeshData`, `MeshGroupProperty`, `MeshFieldMapping` model nodes/elements/properties; loaders such as `RWVtk::VtkBaseReader`, `RWMed::RWMedReader`, `RWMeshExchanger::MeshExchanger`, `RWMeshConv::MedConverter` import/export multiple formats.
- LOD/Simplification: `LodCae::Simplifier` + `SimMesh_Model` (source/SimMesh/SimMesh_Model.h:20) + `SimMesh_Simplifier` build domain-based LOD meshes.

### 4.3 Visualization Services
- `ViewBase::IInteractiveObject` extends AIS with DPI/pixel callbacks and mouse hooks; `ViewBase::InteractiveOwner` (source/ViewBase/InteractiveOwner.h:21) centralizes selection priority and picking tolerance.
- `ViewShape::InteractiveShape` handles per-material aspects and sub-shape state; `ViewCae::InteractiveCaeMesh` orchestrates `MeshPresentation` instances, color scales, and playback through `ViewCae::CaeMeshAnimation` (source/ViewCae/CaeMeshAnimation.h:13); `ViewDim::*` adds measurement overlays.
- `ViewCaeGeom` (Clip/Iso/Slice/Deformed/etc.) turns `RWCae::MeshData` into GPU buffers; `ViewBase` also provides `InteractiveViewCube`, `InteractiveScaleRuler`, `OffscreenViewer`, `StatsActivator` helpers.

### 4.4 Viewer Runtime
- `Viewer::WasmViewer` initializes `V3d_View`/`AIS_InteractiveContext`, wires `ManagerType` instances, handles input events, font registration, and rendering parameters.
- `Viewer::InteractiveContext` (source/Viewer/InteractiveContext.h:16) tracks life-cycle (visibility, loading state, view corner, priority modifiers).
- `Viewer::SelectionManager` (source/Viewer/SelectionManager.h:12) stores object/sub-shape/mesh selections and exposes serialization to TS.
- `Viewer::ViewManager` (source/Viewer/ViewManager.h:45) manages multi-view layouts, gestures, fit/orientation logic.
- TypeScript API is described in the .d.ts file; `Viewer::Utils::SelectionMeshEntity` (source/Viewer/Utils/SelectionMeshEntity.h:37) is registered via Emscripten bindings so JS can inspect picked entities.

## 5. Key Flows
### 5.1 B-Rep Loading
1. `RWShapeConv::ShapeLoader` populates `RWShape::ShapeData` from glTF/BREP.
2. `Viewer::InteractiveContext::LoadData()` calls `RWBase::TaskManager::RunAsync()` for IO+tessellation work.
3. Main-thread callback instantiates `ViewShape::InteractiveShape`, registers `InteractiveOwner`, updates `SelectionManager` caches.

### 5.2 CAE Mesh & Animation
1. `MedConverter` / `RWVtk` / `RWMeshExchanger` parse mesh + field mappings (`RWCae::MeshFieldMapping`).
2. `ViewCae::InteractiveCaeMesh` builds presentations per type, and `ViewCaeGeom::*` emit GPU primitives.
3. `ViewCae::CaeMeshAnimation` advances property time steps; heavy updates run asynchronously and resync color mappers/presentations once complete.

### 5.3 Interaction & Selection
1. Emscripten events → `WasmViewer::processKeyPress/onMouseEvent` → `AIS_ViewController` navigation.
2. `SelectionManager` uses `ViewBase::InteractiveOwner` + `Viewer::Utils::SelectionMeshEntity` to expose B-Rep sub-shapes and CAE mesh entities.
3. Promise callbacks (`RWBase::IPromise`) notify the TS layer when loads/selections finish.

## 6. Collaboration Highlights
- `Viewer::ObjectId` (source/Viewer/Utils/ObjectId.h:9) uniquely identifies objects across owners, AIS instances, and TS APIs.
- Owner pattern: `InteractiveOwner` encapsulates selection handles/priorities; `InteractiveShapeListOwner` supports multiple shapes per object.
- Async constraints: `TaskManager` must degrade gracefully when Wasm lacks PThreads (`SetMultithreadingMode(false)`), often synchronized through `RWBase::ConditionPromise`.
- Property mapping: `RWCae::MeshFieldMapping` + `MeshColorMapper` translate solver data into textures/colors; `ViewCae::ColorScaleTools` (source/ViewCae/ColorScaleTools.h:12) renders the legends.
- Web API glue: `Viewer::Utils::EmscriptenHelper` (source/Viewer/Utils/EmscriptenHelper.h:10) converts vectors/maps between C++ and JS.

## 7. Risks & Suggestions
1. Thread model: ensure `TaskManager::RunInParallel` is disabled when Wasm builds lack pthreads.
2. Virtual FS: `EmulatedFileSystem` currently handles a single file; consider extending `FileSystemMap` for multi-file bundles.
3. Mapping errors: expose `MedConverter` `ConversionStatus::MissingFields` in UI for better diagnostics.
4. Memory pressure: large shapes/meshes cached in `MeshPresentation`/`InteractiveShape` should leverage LOD (`LodCae::Simplifier`) or cache toggles (`Presentation()->SetCachePrimitives`).
5. TS API docs: generate examples/docs from the .d.ts definitions to reduce integration friction.
