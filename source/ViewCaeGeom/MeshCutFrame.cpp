#include <ViewCaeGeom/MeshCutFrame.h>
#include <cmath>
#include <stdexcept>

namespace ViewCaeGeom {
bool MeshCutFrame::IsValid() const
{
    for (const auto& v : {Origin,X,Y,Z}) {
        for (int i = 0; i < 3; ++i) { if (!std::isfinite(v[i])) { return false; } }
    }
    constexpr double tolerance = 1e-9;
    return std::abs(X.SquareModulus()-1) < tolerance && std::abs(Y.SquareModulus()-1) < tolerance
        && std::abs(Z.SquareModulus()-1) < tolerance && std::abs(X.Dot(Y)) < tolerance
        && (Graphic3d_Vec3d::Cross(X,Y)-Z).Modulus() < tolerance;
}
Graphic3d_Vec3d MeshCutFrame::VectorToGlobal(const Graphic3d_Vec3d& v) const
{
    if (!IsValid()) { throw std::invalid_argument("Datum must be finite, orthonormal and right handed"); }
    return X*v.x()+Y*v.y()+Z*v.z();
}
Graphic3d_Vec3d MeshCutFrame::VectorToLocal(const Graphic3d_Vec3d& v) const
{
    if (!IsValid()) { throw std::invalid_argument("Invalid datum"); }
    return Graphic3d_Vec3d(v.Dot(X),v.Dot(Y),v.Dot(Z));
}
Graphic3d_Vec3d MeshCutFrame::PointToGlobal(const Graphic3d_Vec3d& v) const { return Origin+VectorToGlobal(v); }
Graphic3d_Vec3d MeshCutFrame::PointToLocal(const Graphic3d_Vec3d& v) const { return VectorToLocal(v-Origin); }
Graphic3d_Vec4d MeshCutFrame::PlaneToGlobal(const Graphic3d_Vec4d& v) const
{
    const auto normal = VectorToGlobal(Graphic3d_Vec3d(v.x(),v.y(),v.z()));
    return Graphic3d_Vec4d(normal.x(),normal.y(),normal.z(),v.w()-normal.Dot(Origin));
}
Graphic3d_Vec4d MeshCutFrame::PlaneToLocal(const Graphic3d_Vec4d& v) const
{
    const Graphic3d_Vec3d normal(v.x(),v.y(),v.z());
    const auto local = VectorToLocal(normal);
    return Graphic3d_Vec4d(local.x(),local.y(),local.z(),v.w()+normal.Dot(Origin));
}
bool MeshCutFrame::Edit(MeshCutViewManager& manager, const MeshCutDefinition& local) const
{
    if (!IsValid()) { return false; }
    switch (local.Shape) {
        case MeshCutShape::Plane: return manager.SetPlane(local.Id,PlaneToGlobal(local.Plane));
        case MeshCutShape::Sphere: return manager.SetSphere(local.Id,PointToGlobal(local.Center),local.Radius);
        case MeshCutShape::Cylinder: return manager.SetCylinder(local.Id,PointToGlobal(local.Center),VectorToGlobal(local.Axis),local.Radius);
        case MeshCutShape::Isosurface: return manager.SetIsoValue(local.Id,local.IsoValue);
    }
    return false;
}
}
