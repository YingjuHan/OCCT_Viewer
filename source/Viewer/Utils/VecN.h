#ifndef _VECN_HEADERFILE
#define _VECN_HEADERFILE

#include <Bnd_Box.hxx>
#include <gp_Ax2.hxx>
#include <gp_XY.hxx>
#include <gp_XYZ.hxx>
#include <Graphic3d_Vec2.hxx>
#include <Graphic3d_Vec3.hxx>
#include <Graphic3d_Vec4.hxx>
#include <Graphic3d_BndBox3d.hxx>

namespace Viewer {
//! Represents vector 2d struct.
class Vec2 : public Graphic3d_Vec2d {
public:
    Vec2() {}
    explicit Vec2(const Graphic3d_Vec2d& theVec) : Graphic3d_Vec2d(theVec) {}
    explicit Vec2(const Graphic3d_Vec2i& theVec) : Graphic3d_Vec2d(theVec) {}
    explicit Vec2(const gp_XY& thePnt) : Graphic3d_Vec2d(thePnt.X(), thePnt.Y()) {}

    double X() const { return x(); }
    void   SetX(double theVal) { x() = theVal; }

    double Y() const { return y(); }
    void   SetY(double theVal) { y() = theVal; }

    gp_XY XY() const;
    void  SetXY(const gp_XY& thePnt);

    //! Return TRUE if vector is infinite.
    bool IsInfinite() const { return Precision::IsInfinite(x()) || Precision::IsInfinite(y()); }
};

//! Represents vector 3d struct.
class Vec3 : public Graphic3d_Vec3d {
public:
    //! Return invalid (infinite) point.
    static Vec3 InvalidPoint() { return Vec3(gp_XYZ(RealLast(), RealLast(), RealLast())); }

    //! Return zero point.
    static Vec3 ZeroPoint() { return Vec3(gp_XYZ(0.0, 0.0, 0.0)); }

public:
    Vec3() {}
    explicit Vec3(const Graphic3d_Vec3d& theVec) : Graphic3d_Vec3d(theVec) {}
    explicit Vec3(const gp_XYZ& thePt) : Graphic3d_Vec3d(thePt.X(), thePt.Y(), thePt.Z()) {}

    double X() const { return x(); }
    void   SetX(double theVal) { x() = theVal; }

    double Y() const { return y(); }
    void   SetY(double theVal) { y() = theVal; }

    double Z() const { return z(); }
    void   SetZ(double theVal) { z() = theVal; }

    gp_XYZ XYZ() const;
    void   SetXYZ(const gp_XYZ& thePnt);

    //! Return TRUE if vector is infinite.
    bool IsInfinite() const
    {
        return Precision::IsInfinite(x()) || Precision::IsInfinite(y()) || Precision::IsInfinite(z());
    }
};

//! Represents vector 4d struct.
class Vec4 : public Graphic3d_Vec4d {
public:
    using Graphic3d_Vec4d::Graphic3d_Vec4d;

    explicit Vec4(const Graphic3d_Vec4d& theVec) : Graphic3d_Vec4d(theVec) {}

    double X() const { return x(); }
    void   SetX(double theVal) { x() = theVal; }

    double Y() const { return y(); }
    void   SetY(double theVal) { y() = theVal; }

    double Z() const { return z(); }
    void   SetZ(double theVal) { z() = theVal; }

    double W() const { return w(); }
    void   SetW(double theVal) { w() = theVal; }

    void SetXYZ(const gp_XYZ& thePnt);

    //! Return TRUE if vector is infinite.
    bool IsInfinite() const
    {
        return Precision::IsInfinite(x()) || Precision::IsInfinite(y()) || Precision::IsInfinite(z())
            || Precision::IsInfinite(w());
    }
};

//! Represents 4x4 matrix struct.
struct Mat4 {
    Vec4 row1;
    Vec4 row2;
    Vec4 row3;
    Vec4 row4;
};

//! Represents an axes-aligned 3D bounding box defined by it's corner points.
//! Invalid (void) bounding box is defined by negative extent.
class BndBox3 : public Graphic3d_BndBox3d {
public:
    //! Empty constructor.
    BndBox3() : Graphic3d_BndBox3d()
    {
        myMinPoint.SetValues(RealLast(), RealLast(), RealLast());
        myMaxPoint.SetValues(-RealLast(), -RealLast(), -RealLast());
        myIsInited = false;
    }

    //! Copy constructor.
    explicit BndBox3(const Graphic3d_BndBox3d& theBox) : Graphic3d_BndBox3d(theBox)
    {
        if (!theBox.IsValid()) {
            myMinPoint.SetValues(RealLast(), RealLast(), RealLast());
            myMaxPoint.SetValues(-RealLast(), -RealLast(), -RealLast());
            myIsInited = false;
        }
    }

    //! Convert from Bnd_Box.
    explicit BndBox3(const Bnd_Box& theBox) : Graphic3d_BndBox3d()
    {
        if (theBox.IsVoid()) {
            myMinPoint.SetValues(RealLast(), RealLast(), RealLast());
            myMaxPoint.SetValues(-RealLast(), -RealLast(), -RealLast());
            myIsInited = false;
        } else {
            myMinPoint = Vec3(theBox.CornerMin().XYZ());
            myMaxPoint = Vec3(theBox.CornerMax().XYZ());
            myIsInited = true;
        }
    }

    //! Convert into Bnd_Box.
    operator Bnd_Box() const
    {
        Bnd_Box aBndBox;
        if (IsValid()) {
            aBndBox.Add(gp_Pnt(GetCornerMin().XYZ()));
            aBndBox.Add(gp_Pnt(GetCornerMax().XYZ()));
        }
        return aBndBox;
    }

    Vec3 GetCornerMin() const { return Vec3(CornerMin()); }
    void SetCornerMin(const Vec3& theVal)
    {
        myMinPoint = theVal;
        myIsInited = myMaxPoint.x() >= myMinPoint.x() && myMaxPoint.y() >= myMinPoint.y()
                  && myMaxPoint.z() >= myMinPoint.z();
    }

    Vec3 GetCornerMax() const { return Vec3(CornerMax()); }
    void SetCornerMax(const Vec3& theVal)
    {
        myMaxPoint = theVal;
        myIsInited = myMaxPoint.x() >= myMinPoint.x() && myMaxPoint.y() >= myMinPoint.y()
                  && myMaxPoint.z() >= myMinPoint.z();
    }
};

//! Object location.
struct Location {
    Location() {}
    Standard_EXPORT Location(const gp_Trsf& theTrsf);

    Vec4   rotation    = Vec4(Graphic3d_Vec4d(0.0, 0.0, 0.0, 1.0)); //!< rotation quaternion
    Vec3   translation = Vec3(Graphic3d_Vec3d(0.0, 0.0, 0.0));      //!< translation vector
    double scale       = 1.0;                                       //!< uniform scale factor
};

//! Axes definition.
struct Ax2 {
public:
    //! Return invalid (infinite) point.
    static Ax2 InvalidAx2()
    {
        Ax2 anAx2;
        anAx2.location   = Vec3::InvalidPoint();
        anAx2.direction  = Vec3::ZeroPoint();
        anAx2.xDirection = Vec3::ZeroPoint();
        return anAx2;
    }

public:
    Vec3 location;
    Vec3 direction;
    Vec3 xDirection;

    Ax2() {}
    explicit Ax2(const gp_Ax2& theAx2)
        : location(theAx2.Location().XYZ()),
          direction(theAx2.Direction().XYZ()),
          xDirection(theAx2.XDirection().XYZ())
    {
    }

    //! Return ax2.
    gp_Ax2 GetAx2() const
    {
        gp_Dir aDir = HasDirection() ? direction.XYZ() : gp::DZ();
        if (HasXDirection()) {
            return gp_Ax2(location.XYZ(), aDir, xDirection.XYZ());
        }
        return gp_Ax2(location.XYZ(), aDir);
    }

    //! Return TRUE if location point is defined.
    bool HasLocation() const { return !location.IsInfinite(); }

    //! Return TRUE if direction vector is defined.
    bool HasDirection() const { return !direction.IsInfinite() && direction.SquareModulus() > gp::Resolution(); }

    //! Return TRUE if direction vector is defined.
    bool HasXDirection() const { return !xDirection.IsInfinite() && xDirection.SquareModulus() > gp::Resolution(); }
};

} // namespace

#endif // _VECN_HEADERFILE
