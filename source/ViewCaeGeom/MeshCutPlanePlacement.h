#ifndef _MESHCUTPLANEPLACEMENT_HEADERFILE
#define _MESHCUTPLANEPLACEMENT_HEADERFILE

#include <Graphic3d_Vec.hxx>
#include <Standard_Macro.hxx>

namespace ViewCaeGeom {
//! Definition axes are immutable during absolute motion. Axis1 is the normal;
//! Axis3 = Axis1 x Axis2. Angles are degrees, lengths are model units.
struct MeshCutPlanePlacement {
    Graphic3d_Vec3d Origin = Graphic3d_Vec3d(0.0);
    Graphic3d_Vec3d Normal = Graphic3d_Vec3d(1,0,0);
    Graphic3d_Vec3d Axis2 = Graphic3d_Vec3d(0,1,0);
    double Offset = 0;
    double Angle = 0;
    int RotationAxis = 2;
    Standard_EXPORT bool IsValid() const;
    //! Normalize perpendicular direction inputs; invalid input leaves this unchanged.
    Standard_EXPORT bool Define(const Graphic3d_Vec3d& origin,
        const Graphic3d_Vec3d& normal, const Graphic3d_Vec3d& axis2);
    //! Deterministic migration for an equation without a historical frame.
    Standard_EXPORT bool FromEquation(const Graphic3d_Vec4d& equation);
    Standard_EXPORT Graphic3d_Vec3d CurrentNormal() const;
    Standard_EXPORT Graphic3d_Vec3d CurrentAxis2() const;
    Standard_EXPORT Graphic3d_Vec3d CurrentOrigin() const;
    Standard_EXPORT Graphic3d_Vec4d Equation() const;
};
}
#endif
