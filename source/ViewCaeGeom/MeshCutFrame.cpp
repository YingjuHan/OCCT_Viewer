#include <ViewCaeGeom/MeshCutFrame.h>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace ViewCaeGeom {
namespace {
bool finite(const Graphic3d_Vec3d& v)
{
    return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
}
Graphic3d_Vec3d rotated(const MeshCutPlanePlacement& p, const Graphic3d_Vec3d& v)
{
    const auto axis = p.RotationAxis == 2 ? p.Axis2 : Graphic3d_Vec3d::Cross(p.Normal,p.Axis2);
    const double angle = std::remainder(p.Angle,360.0)*std::acos(-1.0)/180.0;
    return v*std::cos(angle)+Graphic3d_Vec3d::Cross(axis,v)*std::sin(angle)
        +axis*axis.Dot(v)*(1-std::cos(angle));
}
}
bool MeshCutPlanePlacement::IsValid() const
{
    return finite(Origin) && finite(Normal) && finite(Axis2) && std::isfinite(Offset) && std::isfinite(Angle)
        && (RotationAxis == 2 || RotationAxis == 3) && std::abs(Normal.SquareModulus()-1) < 1e-9
        && std::abs(Axis2.SquareModulus()-1) < 1e-9 && std::abs(Normal.Dot(Axis2)) < 1e-9;
}
bool MeshCutPlanePlacement::Define(const Graphic3d_Vec3d& origin,
    const Graphic3d_Vec3d& normal, const Graphic3d_Vec3d& axis2)
{
    const double n = std::hypot(normal.x(),normal.y(),normal.z()), a = std::hypot(axis2.x(),axis2.y(),axis2.z());
    if (!finite(origin) || !std::isfinite(n) || !std::isfinite(a) || n <= 1e-12 || a <= 1e-12) { return false; }
    MeshCutPlanePlacement p; p.Origin = origin; p.Normal = normal/n; p.Axis2 = axis2/a;
    if (!p.IsValid() || !std::isfinite(p.Equation().w())) { return false; }
    *this = p; return true;
}
bool MeshCutPlanePlacement::FromEquation(const Graphic3d_Vec4d& equation)
{
    const double length = std::hypot(equation.x(),equation.y(),equation.z());
    if (!std::isfinite(length) || length <= 1e-300 || !std::isfinite(equation.w())) { return false; }
    const Graphic3d_Vec3d n = Graphic3d_Vec3d(equation.x(),equation.y(),equation.z())/length;
    int component = 0;
    for (int i = 1; i < 3; ++i) { if (std::abs(n[i]) < std::abs(n[component])) { component = i; } }
    Graphic3d_Vec3d seed(0.0); seed[component] = 1;
    return Define(n*(-equation.w()/length),n,(seed-n*n.Dot(seed)).Normalized());
}
Graphic3d_Vec3d MeshCutPlanePlacement::CurrentNormal() const { return rotated(*this,Normal); }
Graphic3d_Vec3d MeshCutPlanePlacement::CurrentAxis2() const { return rotated(*this,Axis2); }
Graphic3d_Vec3d MeshCutPlanePlacement::CurrentOrigin() const { return Origin+CurrentNormal()*Offset; }
Graphic3d_Vec4d MeshCutPlanePlacement::Equation() const
{
    const auto n = CurrentNormal();
    return Graphic3d_Vec4d(n.x(),n.y(),n.z(),-n.Dot(Origin)-Offset);
}
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
        case MeshCutShape::Plane: {
            const auto equation = local.Placement.Equation();
            bool complete = local.Placement.IsValid();
            for (int i = 0; i < 4; ++i) {
                complete = complete && std::abs(equation[i]-local.Plane[i]) <= 1e-12*std::max(1.0,std::abs(local.Plane[i]));
            }
            return complete ? manager.SetPlanePlacement(local.Id,PlacementToGlobal(local.Placement))
                : manager.SetPlane(local.Id,PlaneToGlobal(local.Plane));
        }
        case MeshCutShape::Sphere: return manager.SetSphere(local.Id,PointToGlobal(local.Center),local.Radius);
        case MeshCutShape::Cylinder: return manager.SetCylinder(local.Id,PointToGlobal(local.Center),VectorToGlobal(local.Axis),local.Radius);
        case MeshCutShape::Isosurface: return manager.SetIsoValue(local.Id,local.IsoValue);
    }
    return false;
}
MeshCutPlanePlacement MeshCutFrame::PlacementToGlobal(MeshCutPlanePlacement p) const
{
    p.Origin = PointToGlobal(p.Origin); p.Normal = VectorToGlobal(p.Normal); p.Axis2 = VectorToGlobal(p.Axis2); return p;
}
MeshCutPlanePlacement MeshCutFrame::PlacementToLocal(MeshCutPlanePlacement p) const
{
    p.Origin = PointToLocal(p.Origin); p.Normal = VectorToLocal(p.Normal); p.Axis2 = VectorToLocal(p.Axis2); return p;
}
}
