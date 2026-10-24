#ifndef _MESHCUTPHYSICS_HEADERFILE
#define _MESHCUTPHYSICS_HEADERFILE
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <ViewCaeGeom/MeshCutFrame.h>
#include <ViewCaeGeom/MeshCutSlicing.h>
#include <set>

namespace ViewCaeGeom {
enum class MeshCutSummation { Signed, AbsoluteContributions };
enum class MeshCutSummationPoint { UserDefined, Centroid };
enum class MeshCutSectionKind { Volume, ShellStress, CompositeStress, BeamStress, BeamResultants };
//! All factors convert the explicitly named input units to SI; output is N, Nm, m2, W.
struct MeshCutUnits {
    std::string Length = "m", Stress = "Pa", Flux = "W/m2", Force = "N", Moment = "Nm";
    double LengthToSI = 1, StressToSI = 1, FluxToSI = 1, ForceToSI = 1, MomentToSI = 1;
    Standard_EXPORT bool IsValid() const;
};
struct MeshCutLayer {
    Handle(RWCae::MeshNVecProperty) Field;
    std::array<int,9> Components = {{0,1,2,3,4,5,6,7,8}};
    double Thickness = 0; //!< Input length units; strictly positive.
    double Offset = 0; //!< Signed mid-layer offset along section frame Z.
    MeshCutFrame FieldFrame; //!< Local tensor components -> global; no inferred conventions.
};
struct MeshCutSectionMapping {
    MeshCutSectionKind Kind = MeshCutSectionKind::Volume;
    double Thickness = 0, Area = 0;
    MeshCutFrame Frame;
    std::vector<MeshCutLayer> Layers;
    //! Beam resultants are [Fx,Fy,Fz,Mx,My,Mz] in Frame, already section-integrated.
    std::array<int,6> BeamComponents = {{0,1,2,3,4,5}};
};
struct MeshCutPhysicsOptions {
    std::string GroupName = "all";
    std::set<RWCae::MeshEntityId> Elements; //!< Empty only means all for GroupName == "all".
    std::function<bool(RWCae::MeshEntityId)> DomainContains; //!< Explicit live domain adapter; false never means whole model.
    MeshCutUnits Units;
    MeshCutFrame OutputFrame; //!< Result vectors and positions expressed in this frame.
    MeshCutFrame FieldFrame; //!< Input vector/tensor frame.
    Graphic3d_Vec3d MomentOrigin = Graphic3d_Vec3d(0); //!< Global, input length units.
    MeshCutSummationPoint SumPoint = MeshCutSummationPoint::UserDefined;
    int NormalSign = 1; //!< +1 Below -> Above; -1 Above -> Below.
    MeshCutSummation Summation = MeshCutSummation::Signed;
    std::map<RWCae::MeshEntityId,MeshCutSectionMapping> Sections;
    Handle(MeshGeometry) SourceGeometry; //!< Explicit configuration for beam intersection positions.
    Handle(MeshGeometry) ReferenceGeometry; //!< Material beam reference crossing; current position still uses SourceGeometry.
    //! Isosurfaces require explicit normal mapping; sphere/cylinder use analytic outward normals.
    std::function<Graphic3d_Vec3d(RWCae::MeshEntityId,const Graphic3d_Vec3d&)> Normal;
    std::map<RWCae::MeshEntityId,Graphic3d_Vec3d> ElementNormals; //!< Explicit serializable normals, e.g. planar distance fields.
};
struct MeshCutDistributedResult {
    RWCae::MeshEntityId Element = 0;
    Graphic3d_Vec3d Position = Graphic3d_Vec3d(0);
    Graphic3d_Vec3d Normal = Graphic3d_Vec3d(0);
    MeshCutResultant Result;
};
struct MeshCutPhysicsReport {
    std::string Cut, Group;
    MeshCutResultant Total;
    Graphic3d_Vec3d Centroid = Graphic3d_Vec3d(0); //!< Output-frame coordinates, metres.
    Graphic3d_Vec3d MomentOrigin = Graphic3d_Vec3d(0); //!< Actual summation point, output frame/metres.
    Graphic3d_Vec3d MeanNormal = Graphic3d_Vec3d(0); //!< Oriented area-weighted direction, output frame.
    MeshCutFrame OutputFrame;
    std::vector<MeshCutDistributedResult> Distributed;
    double IntervalBegin = 0, IntervalEnd = 0;
    std::string IntervalUnits = "model_length";
    Standard_EXPORT std::string Csv() const;
};
//! Independent physical fields and selected physical FE provenance only.
//! Volume faces, shell co-normal/thickness, explicit composite layers and beam sections.
class MeshCutPhysics {
public:
    Standard_EXPORT static MeshCutPhysicsReport Stress(const MeshCutResult& cut,
        const std::array<int,9>& components, const MeshCutPhysicsOptions& options);
    Standard_EXPORT static MeshCutPhysicsReport HeatFlux(const MeshCutResult& cut,
        const std::array<int,3>& components, const MeshCutPhysicsOptions& options);
    //! Beam intersections do not depend on the optional VTK surface-cut backend.
    Standard_EXPORT static MeshCutPhysicsReport Beam(const MeshCutDefinition& plane,
        const Handle(RWCae::MeshNVecProperty)& field, const std::array<int,9>& components,
        const MeshCutPhysicsOptions& options);
    Standard_EXPORT static std::vector<MeshCutPhysicsReport> Slices(MeshCutViewManager& manager,
        const std::vector<MeshCutSliceSample>& samples, const Handle(RWCae::MeshNVecProperty)& field,
        const std::array<int,9>& components, const MeshCutPhysicsOptions& options);
};
}
#endif
