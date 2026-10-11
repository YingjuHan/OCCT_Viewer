#ifndef _MESHCUTINTEGRATION_HEADERFILE
#define _MESHCUTINTEGRATION_HEADERFILE

#include <ViewCaeGeom/MeshCutViewManager.h>

namespace ViewCaeGeom {
struct MeshCutResultant {
    Graphic3d_Vec3d Force = Graphic3d_Vec3d(0.0);
    Graphic3d_Vec3d Moment = Graphic3d_Vec3d(0.0);
    double Area = 0.0;
    double HeatFlow = 0.0;
};

//! Integrates volume-cell section faces using the positive plane normal.
//! Component maps index physical components, excluding generated vector magnitude.
//! Shell/beam section resultants need an explicit thickness/section-field adapter.
class MeshCutIntegration {
public:
    Standard_EXPORT static MeshCutResultant Stress(const MeshCutResult& theCut,
        const std::array<int, 9>& theTensorComponents, const Graphic3d_Vec3d& theMomentOrigin);
    Standard_EXPORT static MeshCutResultant HeatFlux(const MeshCutResult& theCut,
        const std::array<int, 3>& theVectorComponents);
};
}
#endif
