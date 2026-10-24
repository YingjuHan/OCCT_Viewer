
#include "VecN.h"

#include <gp_Quaternion.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {
// =======================================================================
// function : XY
// =======================================================================
gp_XY Vec2::XY() const
{
    return gp_XY(x(), y());
}

// =======================================================================
// function : SetXY
// =======================================================================
void Vec2::SetXY(const gp_XY& thePnt)
{
    SetValues(thePnt.X(), thePnt.Y());
}

// =======================================================================
// function : XYZ
// =======================================================================
gp_XYZ Vec3::XYZ() const
{
    return gp_XYZ(x(), y(), z());
}

// =======================================================================
// function : SetXYZ
// =======================================================================
void Vec3::SetXYZ(const gp_XYZ& thePnt)
{
    SetValues(thePnt.X(), thePnt.Y(), thePnt.Z());
}

// =======================================================================
// function : SetXYZ
// =======================================================================
void Vec4::SetXYZ(const gp_XYZ& thePnt)
{
    SetValues(thePnt.X(), thePnt.Y(), thePnt.Z(), w());
}

// ================================================================
// Function : Location
// ================================================================
Location::Location(const gp_Trsf& theTrsf)
{
    if (theTrsf.Form() == gp_Identity) {
        return;
    }

    gp_Quaternion aRotQ = theTrsf.GetRotation();
    translation         = Vec3(theTrsf.TranslationPart());
    rotation            = Vec4(aRotQ.X(), aRotQ.Y(), aRotQ.Z(), aRotQ.W());
    scale               = theTrsf.ScaleFactor();
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(Vec2)
{
    emscripten::value_object<Vec2>("Vec2").field("x", &Vec2::X, &Vec2::SetX).field("y", &Vec2::Y, &Vec2::SetY);
}
EMSCRIPTEN_BINDINGS(Vec3)
{
    emscripten::value_object<Vec3>("Vec3")
        .field("x", &Vec3::X, &Vec3::SetX)
        .field("y", &Vec3::Y, &Vec3::SetY)
        .field("z", &Vec3::Z, &Vec3::SetZ);

    emscripten::register_vector<Vec3>("Vec3Vector");
}
EMSCRIPTEN_BINDINGS(Vec4)
{
    emscripten::value_object<Vec4>("Vec4")
        .field("x", &Vec4::X, &Vec4::SetX)
        .field("y", &Vec4::Y, &Vec4::SetY)
        .field("z", &Vec4::Z, &Vec4::SetZ)
        .field("w", &Vec4::W, &Vec4::SetW);
}
EMSCRIPTEN_BINDINGS(Mat4)
{
    emscripten::value_object<Mat4>("Mat4")
        .field("row1", &Mat4::row1)
        .field("row2", &Mat4::row2)
        .field("row3", &Mat4::row3)
        .field("row4", &Mat4::row4);
}
EMSCRIPTEN_BINDINGS(BndBox3)
{
    emscripten::value_object<BndBox3>("BndBox3")
        .field("cornerMin", &BndBox3::GetCornerMin, &BndBox3::SetCornerMin)
        .field("cornerMax", &BndBox3::GetCornerMax, &BndBox3::SetCornerMax);
}
EMSCRIPTEN_BINDINGS(Location)
{
    emscripten::value_object<Location>("Location")
        .field("rotation", &Location::rotation)
        .field("translation", &Location::translation)
        .field("scale", &Location::scale);
}
EMSCRIPTEN_BINDINGS(Ax2)
{
    emscripten::value_object<Ax2>("Ax2")
        .field("location", &Ax2::location)
        .field("direction", &Ax2::direction)
        .field("xDirection", &Ax2::xDirection);
}
#endif

} // namespace
