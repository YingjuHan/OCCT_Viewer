#ifndef _MESHCUTFRAME_HEADERFILE
#define _MESHCUTFRAME_HEADERFILE

#include <ViewCaeGeom/MeshCutViewManager.h>

namespace ViewCaeGeom {
//! Right-handed orthonormal datum. Coordinates and lengths use model units.
struct MeshCutFrame {
    Graphic3d_Vec3d Origin = Graphic3d_Vec3d(0.0);
    Graphic3d_Vec3d X = Graphic3d_Vec3d(1,0,0);
    Graphic3d_Vec3d Y = Graphic3d_Vec3d(0,1,0);
    Graphic3d_Vec3d Z = Graphic3d_Vec3d(0,0,1);
    Standard_EXPORT bool IsValid() const;
    Standard_EXPORT Graphic3d_Vec3d VectorToGlobal(const Graphic3d_Vec3d& value) const;
    Standard_EXPORT Graphic3d_Vec3d VectorToLocal(const Graphic3d_Vec3d& value) const;
    Standard_EXPORT Graphic3d_Vec3d PointToGlobal(const Graphic3d_Vec3d& value) const;
    Standard_EXPORT Graphic3d_Vec3d PointToLocal(const Graphic3d_Vec3d& value) const;
    Standard_EXPORT Graphic3d_Vec4d PlaneToGlobal(const Graphic3d_Vec4d& value) const;
    Standard_EXPORT Graphic3d_Vec4d PlaneToLocal(const Graphic3d_Vec4d& value) const;
    Standard_EXPORT MeshCutPlanePlacement PlacementToGlobal(MeshCutPlanePlacement value) const;
    Standard_EXPORT MeshCutPlanePlacement PlacementToLocal(MeshCutPlanePlacement value) const;
    //! Uses the manager's validation/revision path, also used by mouse manipulators.
    Standard_EXPORT bool Edit(MeshCutViewManager& manager, const MeshCutDefinition& local) const;
};
}
#endif
