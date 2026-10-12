#ifndef _MESHCUTVIEWMANAGER_HEADERFILE
#define _MESHCUTVIEWMANAGER_HEADERFILE

#include <ViewCaeGeom/MeshGeometryClip.h>
#include <ViewCaeGeom/MeshGeometrySlice.h>
#include <ViewCaeGeom/MeshCutRecovery.h>
#include <ViewCaeGeom/MeshCutTraceOptions.h>
#include <ViewCaeGeom/MeshCutCache.h>
#include <Bnd_Box.hxx>

#include <array>
#include <map>
#include <memory>
#include <string>
#include <functional>

namespace ViewCaeGeom {

enum class MeshCutPortion { Negative = 0, On = 1, Positive = 2 };
enum class MeshCutCellMode { Constant, Projected };
enum class MeshCutShape { Plane, Isosurface, Cylinder, Sphere };
enum class MeshCutGeometryMode { Numerical, AnalyticPreferred, AnalyticRequired };

struct MeshCutGeometryOptions {
    MeshCutGeometryMode Mode = MeshCutGeometryMode::AnalyticPreferred;
    double AbsoluteTolerance = 1e-6;
    double RelativeTolerance = 1e-3;
    int FaceSubdivisionLimit = 7;
    int LineSubdivisionLimit = 12;
    uint64_t MaxSourcePrimitives = 2000000;
    Standard_EXPORT bool IsValid() const;
    Standard_EXPORT double Tolerance(double theScale) const;
};

struct MeshCutStatistics {
    uint64_t SourceEpoch = 1;
    uint64_t SourcePreparations = 0;
    uint64_t PreparedCells = 0;
    uint64_t ShapeBuilds = 0;
    uint64_t BvhBuilds = 0;
    uint64_t BoxTests = 0;
    uint64_t CandidateCells = 0;
    uint64_t BooleanOperations = 0;
    uint64_t FilteredFaces = 0;
    uint64_t UnmergedFaceGroups = 0;
    uint64_t SourcePrimitives = 0;
    uint64_t FaceSubdivisionDepth = 0;
    double GeometryTolerance = 0;
    double StorageResolution = 0;
    uint64_t AnalyticCells = 0;
    uint64_t NumericalCells = 0;
    double AnalyticResidual = 0;
};
class MeshVtkCutSource;
class MeshCutTraceSource;
struct MeshCutPresentation;
struct MeshCutPrimitiveTrace;

//! Sampled numerical isosurface controls. Field tolerances have field units;
//! geometry tolerances remain in model units. This is not a certified root isolation.
struct MeshCutIsoOptions {
    double AbsoluteTolerance = 1e-6;
    double RelativeTolerance = 1e-3;
    int MinimumDepth = 2;
    int SubdivisionLimit = 6;
    uint64_t MaxPrimitives = 2000000;
    Standard_EXPORT bool IsValid() const;
};

struct MeshCutDefinition {
    std::string Id;
    std::string Name;
    Graphic3d_Vec4d Plane;
    unsigned Visibility = 2; //!< bits: negative=1, on=2, positive=4
    bool Enabled = true;
    MeshCutCellMode CellMode = MeshCutCellMode::Constant;
    MeshCutTraceOptions DisplayTrace;
    MeshCutTraceOptions PhysicalTrace;
    MeshCutShape Shape = MeshCutShape::Plane;
    std::string IsoFieldName;
    bool IsoFieldNodal = true;
    int IsoComponent = 0; //!< raw component, independent of the displayed contour
    int IsoTimeStep = -1;
    double IsoValue = 0.0;
    Graphic3d_Vec3d Center = Graphic3d_Vec3d(0.0);
    Graphic3d_Vec3d Axis = Graphic3d_Vec3d(0.0, 0.0, 1.0);
    double Radius = 1.0;
};

//! Geometry is live until the next manager update, not an immutable snapshot.
//! Fast native plane/iso paths retain volume cells; general VTK cuts return boundaries.
//! BoundaryMesh distinguishes boundary triangles from replacement volume cells.
struct MeshCutResult {
    MeshCutDefinition Definition;
    uint64_t Revision = 0;
    std::array<Handle(MeshGeometry), 3> Roots;
    std::array<Handle(MeshGeometry), 3> Portions;
    std::array<Handle(RWCae::MeshNVecProperty), 3> Fields;
    std::array<Handle(RWCae::MeshNVecProperty), 3> PhysicalFields;
    Handle(RWCae::MeshNVecProperty) SourceField;
    Handle(RWCae::MeshNVecProperty) LineField;
    Handle(RWCae::MeshNVecProperty) PhysicalLineField;
    //! Original FE owner/weights under physical trace selection, independent of display policy.
    std::function<const MeshCutPrimitiveTrace&(size_t,bool)> PhysicalProvenance;
    //! OCCT measures of the cutting tool and source boundary (tessellated for general cells).
    std::array<double, 3> Volumes = {{0.0, 0.0, 0.0}};
    std::array<double, 3> Areas = {{0.0, 0.0, 0.0}};
    bool BoundaryMesh = false;
    bool PresentationOnly = false; //!< Union output cannot be used for physical integration.
    bool SourceApproximate = false;
    bool DisplayRecovered = false;
    bool TensorDerived = false; //!< Display component is zero; PhysicalFields still retain original tensors.
    double IsoFieldTolerance = 0;
    double TraceTolerance = 0; //!< Same model/field-unit tolerance used by independent trace selection.
    double IsoFieldResidual = 0;
    double IsoGeometryResidual = 0;
    int IsoSubdivisionDepth = 0;
    uint64_t IsoPrimitives = 0;
};

//! Native cut management. Cuts are independent additive branches, not chained clips.
//! Call InvalidateFields/InvalidateGeometry after mutating source data in place.
class MeshCutViewManager {
public:
    Standard_EXPORT explicit MeshCutViewManager(const Handle(RWCae::MeshData)& theMesh);
    Standard_EXPORT ~MeshCutViewManager();
    MeshCutViewManager(const MeshCutViewManager&) = delete;
    MeshCutViewManager& operator=(const MeshCutViewManager&) = delete;

    Standard_EXPORT bool AddPlane(const std::string& theId, const Graphic3d_Vec4d& thePlane);
    Standard_EXPORT bool SetPlane(const std::string& theId, const Graphic3d_Vec4d& thePlane);
    //! Creates inactive central X/Y/Z planes only when no definitions exist.
    Standard_EXPORT bool EnsureDefaultPlanes();
    //! Scalar cutting; VTK chart cells use bounded original-FE numerical refinement.
    //! Elemental cut fields use equal-weight nodal projection.
    //! The field is registered with the source mesh for durable JSON binding.
    Standard_EXPORT bool AddIsosurface(const std::string& theId,
        const Handle(RWCae::MeshNVecProperty)& theField, int theComponent, double theValue);
    Standard_EXPORT bool SetIsosurface(const std::string& theId,
        const Handle(RWCae::MeshNVecProperty)& theField, int theComponent, double theValue);
    Standard_EXPORT bool SetIsoValue(const std::string& theId, double theValue);
    Standard_EXPORT bool IsoRange(const std::string& theId, double& theMinimum, double& theMaximum);
    //! General cells require OCCTV_USE_VTK; otherwise only linear triangles/tetrahedra.
    //! Radius is in model units.
    Standard_EXPORT bool AddSphere(const std::string& theId, const Graphic3d_Vec3d& theCenter, double theRadius);
    Standard_EXPORT bool SetSphere(const std::string& theId, const Graphic3d_Vec3d& theCenter, double theRadius);
    Standard_EXPORT bool AddCylinder(const std::string& theId, const Graphic3d_Vec3d& theCenter,
        const Graphic3d_Vec3d& theAxis, double theRadius);
    Standard_EXPORT bool SetCylinder(const std::string& theId, const Graphic3d_Vec3d& theCenter,
        const Graphic3d_Vec3d& theAxis, double theRadius);
    Standard_EXPORT bool Rename(const std::string& theId, const std::string& theName);
    Standard_EXPORT bool Duplicate(const std::string& theId, const std::string& theNewId);
    Standard_EXPORT bool Remove(const std::string& theId);
    Standard_EXPORT bool SetVisibility(const std::string& theId, unsigned theMask);
    Standard_EXPORT bool SetEnabled(const std::string& theId, bool theEnabled);
    Standard_EXPORT bool SetCellMode(const std::string& theId, MeshCutCellMode theMode);
    Standard_EXPORT bool SetTraceOptions(const std::string& theId, bool thePhysical,
        const MeshCutTraceOptions& theOptions);
    Standard_EXPORT std::vector<MeshCutDefinition> Definitions() const;
    Standard_EXPORT std::string DumpDefinitionsJson() const;
    //! Rejects invalid documents without changing current cuts.
    Standard_EXPORT bool RestoreDefinitionsJson(const std::string& theJson);
    //! Scale follows the existing model-size-normalized deformation convention.
    Standard_EXPORT bool SetDeformation(const Handle(RWCae::MeshNVecProperty)& theField, double theScale);
    Standard_EXPORT double EffectiveDeformationScale() const;
    const Handle(RWCae::MeshNVecProperty)& DeformationField() const { return myDeformationField; }
    double DeformationScale() const { return myDeformationScale; }

    //! Computes only enabled portions, sharing source mesh storage and cached BVHs.
    Standard_EXPORT MeshCutResult Evaluate(const std::string& theId,
        const Handle(RWCae::MeshNVecProperty)& theField = nullptr);
    //! Map only original physical components, independent of display recovery/measures.
    Standard_EXPORT MeshCutResult EvaluatePhysical(const std::string& theId,
        const Handle(RWCae::MeshNVecProperty)& theField);
    Standard_EXPORT std::vector<MeshCutResult> EvaluateVisible(
        const Handle(RWCae::MeshNVecProperty)& theField = nullptr);
    //! Actual OR of visible regions; independent numerical cuts remain cached separately.
    Standard_EXPORT std::shared_ptr<MeshCutPresentation> EvaluatePresentation(
        const Handle(RWCae::MeshNVecProperty)& theField = nullptr);
    //! Full source geometry, respecting deformation, when no cut is active.
    Standard_EXPORT Handle(MeshGeometry) SourceGeometry();
    //! Nodal bounds of the source after its current deformation modifier.
    Standard_EXPORT Bnd_Box SourceBounds();
    Standard_EXPORT void InvalidateFields();
    Standard_EXPORT void InvalidateGeometry();
    Standard_EXPORT void Clear();
    //! Cumulative work in the current source epoch; field/cut edits retain source preparation.
    Standard_EXPORT MeshCutStatistics Statistics() const;
    //! Invalid options preserve the current source epoch and results.
    Standard_EXPORT bool SetGeometryOptions(const MeshCutGeometryOptions& theOptions);
    const MeshCutGeometryOptions& GeometryOptions() const { return myGeometryOptions; }
    Standard_EXPORT bool SetRecoveryOptions(const MeshCutRecoveryOptions& theOptions);
    const MeshCutRecoveryOptions& RecoveryOptions() const { return myRecoveryOptions; }
    Standard_EXPORT bool SetIsoOptions(const MeshCutIsoOptions& theOptions);
    const MeshCutIsoOptions& IsoOptions() const { return myIsoOptions; }
    //! Share accounting across atomic model/step candidates, before allocating caches.
    Standard_EXPORT bool ShareCacheFrom(const MeshCutViewManager& other);
    bool SetCachePolicy(const MeshCutCachePolicy& policy) { return myCache.SetPolicy(policy); }
    MeshCutCachePolicy CachePolicy() const { return myCache.Policy(); }
    MeshCutCacheStatistics CacheStatistics() const { return myCache.Statistics(); }
    Standard_EXPORT uint64_t EstimatedSourceBytes() const;
    Standard_EXPORT uint64_t EstimatedResultBytes(const Handle(RWCae::MeshNVecProperty)& field=nullptr) const;
    //! New request invalidates every earlier request. Cancellation is thread safe;
    //! geometry/OCCT work and publication remain on the Viewer thread.
    Standard_EXPORT uint64_t BeginRequest(const std::shared_ptr<MeshCutCancellation>& token);
    Standard_EXPORT void CancelRequest();
    uint64_t RequestGeneration() const { return myRequestGeneration; }
    bool IsCurrentRequest(uint64_t generation) const { return generation==myRequestGeneration && (!myCancellation || !myCancellation->Cancelled.load()); }
    const MeshCutFrameTimings& LastTimings() const { return myTimings; }

private:
    MeshCutResult evaluate(const std::string& theId, const Handle(RWCae::MeshNVecProperty)& theField, bool theDisplay);
    struct Entry;
    Entry* find(const std::string& theId);
    void release(Entry& theEntry);
    void initialize(Entry& theEntry);
    Handle(MeshGeometry) makeSourceGeometry(bool theBoundary = true);
    bool moveCachedPlane(Entry& theEntry, const Graphic3d_Vec4d& thePlane);
    bool validIsoField(const Handle(RWCae::MeshNVecProperty)& theField, int theComponent) const;
    bool bindIsoField(Entry& theEntry, const Handle(RWCae::MeshNVecProperty)& theField,
        int theComponent, double theValue);
    Handle(RWCae::MeshNVecProperty) resolveIsoField(const MeshCutDefinition& theDefinition) const;
    Handle(RWCae::MeshScalarPropertyBase) isoValues(Entry& theEntry);
    Handle(RWCae::MeshNVecProperty) project(const Handle(RWCae::MeshNVecProperty)& theField);
    bool validQuadricMesh() const;
    bool needsGeneralPlane() const;
    bool needsGeneralIso() const;
    bool setQuadric(Entry& theEntry, MeshCutShape theShape, const Graphic3d_Vec3d& theCenter,
        const Graphic3d_Vec3d& theAxis, double theRadius);
#ifdef OCCTV_USE_VTK
    std::shared_ptr<MeshVtkCutSource> generalSource();
    std::shared_ptr<MeshVtkCutSource> myGeneralSource;
#endif
    uint64_t mySourceEpoch = 1;
    MeshCutGeometryOptions myGeometryOptions;
    MeshCutRecoveryOptions myRecoveryOptions;
    MeshCutIsoOptions myIsoOptions;
    std::shared_ptr<MeshCutTraceSource> myTraceSource;

    Handle(RWCae::MeshData) myMesh;
    Handle(MeshBvhData) myBvh;
    Handle(MeshGeometry) mySourceGeometry;
    std::map<std::string, std::unique_ptr<Entry>> myEntries;
    Handle(RWCae::MeshNVecProperty) myProjectedSource;
    Handle(RWCae::MeshNVecProperty) myProjectedField;
    Handle(RWCae::MeshNVecProperty) myDeformationField;
    double myDeformationScale = 0.0;
    MeshCutCache myCache;
    std::shared_ptr<void> mySourceLease;
    std::shared_ptr<MeshCutCancellation> myCancellation;
    uint64_t myRequestGeneration=1,myAccess=0,mySourceReserved=0;
    void reserveSource();
    MeshCutFrameTimings myTimings;
    void checkRequest(uint64_t generation) const;
    void prepareCache(Entry& entry,const Handle(RWCae::MeshNVecProperty)& field);
};

}
#endif
