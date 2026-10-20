#ifndef _MESHCUTSLICING_HEADERFILE
#define _MESHCUTSLICING_HEADERFILE
#include <ViewCaeGeom/MeshCutFrame.h>

namespace ViewCaeGeom {
struct MeshCutSliceSample {
    MeshCutDefinition Definition;
    MeshCutFrame Frame;
    double Coordinate = 0;
    double IntervalBegin = 0;
    double IntervalEnd = 0;
};
//! Deterministic slice definitions and midpoint intervals; distances use model units.
//! Rotation coordinates use radians. Path frames use parallel transport.
class MeshCutSlicing {
public:
    Standard_EXPORT static std::vector<MeshCutSliceSample> Regular(const MeshCutDefinition& plane,
        double start, double spacing, size_t count);
    Standard_EXPORT static std::vector<MeshCutSliceSample> Range(const MeshCutDefinition& plane,
        double start, double end, size_t count);
    Standard_EXPORT static std::vector<MeshCutSliceSample> Rotational(const MeshCutDefinition& plane,
        const Graphic3d_Vec3d& axis, const Graphic3d_Vec3d& origin, double start, double end, size_t count);
    Standard_EXPORT static std::vector<MeshCutSliceSample> Path(const std::string& prefix,
        const std::vector<Graphic3d_Vec3d>& points, const Graphic3d_Vec3d& up);
    //! Equal arc-length samples of a polyline, including both endpoints.
    Standard_EXPORT static std::vector<MeshCutSliceSample> ResampledPath(const std::string& prefix,
        const std::vector<Graphic3d_Vec3d>& points, const Graphic3d_Vec3d& up, size_t count);
};
}
#endif
